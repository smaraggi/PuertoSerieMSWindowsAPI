#include <PuertoSerie.h>

PuertoSerie::PuertoSerie(std::string nombrePuerto, DCB parametrosPuerto, COMMTIMEOUTS timeoutsPuerto, int largoMensajeARecibir)
{
	this->largo_mensaje_a_recibir = largoMensajeARecibir;
	this->finalizarEscucha = false;
	this->id_thread_escucha = -1;

	this->nombre_puerto = nombrePuerto;
	this->parametros_puerto_serie = parametrosPuerto;
	this->timeouts = timeoutsPuerto;

	this->funcion_procesar_mensaje = NULL;
	this->handle_puerto_serie = NULL;
	this->handle_evento_fin_escucha_mensajes = CreateEvent(0,0,0,0);
	handle_thread_escucha_mensajes = NULL;
}

PuertoSerie::~PuertoSerie()
{
	if (this->handle_puerto_serie)
	{
		CloseHandle(this->handle_puerto_serie);
	}

	CloseHandle(this->handle_evento_fin_escucha_mensajes);
}

bool PuertoSerie::abrirConexionPuertoSerie()
{
	// Abrimos puerto serie
	this->handle_puerto_serie = CreateFile(this->nombre_puerto.c_str(), 
											GENERIC_READ | GENERIC_WRITE,
											0,
											0,
											OPEN_EXISTING,
											FILE_FLAG_OVERLAPPED, // 0,
											NULL);
	bool resultado = true;
	if (this->handle_puerto_serie == INVALID_HANDLE_VALUE)
	{
		resultado = false;
	}

	// Suscribimos al evento de la llegada de un "char", Windows API
	SetCommMask(this->handle_puerto_serie, EV_RXCHAR | EV_RXCHAR);

	// Cargamos parámetros
	DCB dcbSerialParams;
	dcbSerialParams.DCBlength = this->parametros_puerto_serie.DCBlength;
	dcbSerialParams = this->parametros_puerto_serie;

	if (!GetCommState(this->handle_puerto_serie, &dcbSerialParams))
	{
		resultado = false;
	}
	else
	{		
		dcbSerialParams.BaudRate = this->parametros_puerto_serie.BaudRate;
		dcbSerialParams.ByteSize = this->parametros_puerto_serie.ByteSize;
		dcbSerialParams.StopBits = this->parametros_puerto_serie.StopBits;
		dcbSerialParams.Parity = this->parametros_puerto_serie.Parity;		

		if (!SetCommTimeouts(this->handle_puerto_serie, &this->timeouts))
		{
			resultado = false;
		}
	}

	return resultado;
}

bool PuertoSerie::cerrarConexionPuertoSerie()
{	
	if (this->handle_puerto_serie)
	{
		CancelIo(this->handle_puerto_serie);
		CloseHandle(this->handle_puerto_serie);
		this->handle_puerto_serie = NULL;
	}
	return true;
}

int PuertoSerie::enviarMensaje(std::string mensaje, int candidad_caracteres)
{
	int bytes_enviados = 0;

	OVERLAPPED ov = {0};
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = CreateEvent(0, TRUE, 0, 0);
	DWORD dwBytesWrote = 0;

	char* buffer_escritura = new char[candidad_caracteres + 1];
	strcpy(buffer_escritura, mensaje.c_str());

	int iRet = WriteFile(this->handle_puerto_serie, buffer_escritura/*mensaje.c_str()*/, candidad_caracteres, &dwBytesWrote, &ov);

	if (iRet == 0)
	{
		WaitForSingleObject(ov.hEvent, INFINITE);

		GetOverlappedResult(this->handle_puerto_serie, &ov, &dwBytesWrote, FALSE);
	}

	delete []buffer_escritura;
	CloseHandle(ov.hEvent);

	bytes_enviados = dwBytesWrote;
	return bytes_enviados;
}

void PuertoSerie::iniciarLoopDeEscuchaDeMensajes()
{
	this->finalizarEscucha = false;

	if (this->handle_puerto_serie)
	{
		LPDWORD id_thread = 0;
		this->handle_thread_escucha_mensajes = CreateThread(NULL,
															0,
															FuncionEscuchaDePuertoSerie,
															(void*)this,
															0,
															id_thread);
	}	

	if (this->handle_thread_escucha_mensajes == NULL)
	{
		int a = 0; // error de creación del hilo de escucha...
	}
}

void PuertoSerie::finalizarLoopDeEscuchaDeMensajes()
{
	SetEvent(this->handle_evento_fin_escucha_mensajes); // se setea también finalizarEscucha = true;

	WaitForSingleObject(this->handle_thread_escucha_mensajes, INFINITE);
}

void PuertoSerie::setFuncionDeProcesamientoDeMensajes(void (*procesarMensajeRecibido) (std::string mensaje))
{
	this->funcion_procesar_mensaje = procesarMensajeRecibido;
}

DWORD WINAPI PuertoSerie::FuncionEscuchaDePuertoSerie(LPVOID handle_puerto)
{
	PuertoSerie* instancia = (PuertoSerie*)handle_puerto;

	int cantidad_caracteres_total = instancia->largo_mensaje_a_recibir + 1;
	
	// Reservamso buffer del doble del tamaño del mensaje
	// Si se lee el bufer del puerto serie con (cantidad_caracteres_total - 1) caracteres se retiene localmente
	// hasta una nueva lectura que complete los 'cantidad_caracteres_total' caracteres. El exceso se mantiene localmente.
	// Windows ni puerto serie garantiza que se lea en bloques completos el bufer, puede llegar una parte (en teorìa)
	// generarse una lectura parcial y luego llegar el resto del mensaje. Se presume que tan bajo volumen de datos tendría 
	// que ser leído en una única operación, pero este algortmo contempla el caso remoto de que no.
	char* szBuff = new char[cantidad_caracteres_total * 2];
	int caracteres_acumulados = 0;

	std::string mensaje_recibido;
	
	szBuff[instancia->largo_mensaje_a_recibir] = '\0';
	DWORD dwBytesRead = 0;

	DWORD dwBytesToRead = (DWORD)instancia->largo_mensaje_a_recibir;
	
	// estructura para eventos "overlapped, Windows API
	OVERLAPPED ov = {0};
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = CreateEvent(0, true, 0, 0);
	DWORD dwEventMask = 0;

	HANDLE handlesEsperaDatos[2];
	handlesEsperaDatos[0] = instancia->handle_evento_fin_escucha_mensajes; // Evento para terminar escucha
	handlesEsperaDatos[1] = ov.hEvent; // Evento llegada de datos
	DWORD dwWait = 0; // retorno función de espera de llegada de datos o fin de escucha puerto serie.

	if (instancia->handle_puerto_serie)
	{
		while (!instancia->finalizarEscucha)
		{
			
			// Nos suscribimos al evento de llegada de un byte
			WaitCommEvent(instancia->handle_puerto_serie, &dwEventMask, &ov);

			dwWait = WaitForMultipleObjects(2, handlesEsperaDatos, FALSE, INFINITE);

			switch(dwWait)
			{
				case WAIT_OBJECT_0:
				{
					instancia->finalizarEscucha = true;
				}
				break;

				case WAIT_OBJECT_0 + 1:
				{
					if (dwEventMask == EV_RXCHAR)
					{
						OVERLAPPED ovRead = {0};
						memset(&ovRead, 0, sizeof(ovRead));
						ovRead.hEvent = CreateEvent(0, true, 0, 0);

						COMSTAT comStat;
						DWORD dwErrors;
						ClearCommError(instancia->handle_puerto_serie, &dwErrors, &comStat);
						int bytes_pendientes_de_lectura_en_buffer_recepcion_puerto_serie = comStat.cbInQue;

						// Esperamos a recibir la cantidad de caracteres que deseamos leer.
						// Se bloquea el hilo hasta que llega la cantidad de caracteres deseada.
						if (!ReadFile(instancia->handle_puerto_serie, szBuff, dwBytesToRead, &dwBytesRead, &ovRead))
						{
							// Procesamiento de la lectura incompleta, esperamos a que termine
							// También esperamos al evento de fin de escucha del puerto serie.

							HANDLE handlesEsperaDatosEnLecturaDeBuffer[2];
							handlesEsperaDatosEnLecturaDeBuffer[0] = instancia->handle_evento_fin_escucha_mensajes; // Evento para terminar escucha
							handlesEsperaDatosEnLecturaDeBuffer[1] = ovRead.hEvent; // Evento llegada de datos
							DWORD dwWaitLectura = 0; // retorno función de espera de llegada de más datos o fin de escucha puerto serie.

							dwWaitLectura = WaitForMultipleObjects(2, handlesEsperaDatosEnLecturaDeBuffer, FALSE, INFINITE);
							// WaitForSingleObject(ovRead.hEvent, INFINITE); // espera solamente de conclusión de operación de lectura

							switch(dwWaitLectura)
							{
								case WAIT_OBJECT_0: // evento instancia->handle_evento_fin_escucha_mensajes
								{
									instancia->finalizarEscucha = true;
								}
								break;

								case WAIT_OBJECT_0 + 1: // evento llegada de un caracter
								{
									// Se vuelve a entrar en el ciclo a la espera la llegada de más caracteres hasta 
									// completar una cantidad "largo_mensaje_a_recibir"

									GetOverlappedResult(instancia->handle_puerto_serie, &ovRead, &dwBytesRead, TRUE);
								}
								break;
							}
						}

						mensaje_recibido.assign(szBuff, dwBytesRead); // , instancia->largo_mensaje_a_recibir);
						instancia->funcion_procesar_mensaje(mensaje_recibido);
						mensaje_recibido = "";

						CloseHandle(ovRead.hEvent);
						ResetEvent(ov.hEvent);
					}
				}
				break; // case

			} // switch
		} // while
	} // if handle

	CloseHandle(ov.hEvent);
	delete []szBuff;
	return 0;
}

bool PuertoSerie::setDataTerminalReady(bool dtr)
{
	DWORD dtr_value = 0;

	if (dtr)
	{
		dtr_value = CLRDTR;
	}
	else
	{
		dtr_value = SETDTR;
	}

	return EscapeCommFunction(this->handle_puerto_serie, dtr_value);
}

bool PuertoSerie::completarOperacionesYVaciarBuffers()
{
	return FlushFileBuffers(this->handle_puerto_serie);
}