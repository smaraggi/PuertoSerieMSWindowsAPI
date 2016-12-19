#include <PuertoSerie.h>

// variable global del programa local
bool mensaje_salir_recibido = false;

void procesarMensajeRecibido (std::string mensaje)
{
	std::cout << std::endl << "------------>>>>>>> Mensaje Recibido: " << mensaje << std::endl;

	if (mensaje == "SALIR")
	{
		std::cout << std::endl << "******** Mensaje SALIR Recibido: ingrese una tecla cualquiera para terminar. ******* " << mensaje << std::endl;

		mensaje_salir_recibido = true;
	}
}

int main()
{
	DCB parametros = {0};	
	parametros.DCBlength = sizeof(parametros);
	parametros.BaudRate = CBR_9600;
	parametros.ByteSize = 8;
	parametros.StopBits = ONESTOPBIT;
	parametros.Parity = NOPARITY;
	
	/*
	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	*/
	COMMTIMEOUTS timeouts = {0};
	timeouts.ReadIntervalTimeout = MAXWORD; // 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	PuertoSerie* puerto = new PuertoSerie("COM1", parametros, timeouts, 5);
	puerto->abrirConexionPuertoSerie();

	puerto->setFuncionDeProcesamientoDeMensajes(procesarMensajeRecibido);
	puerto->iniciarLoopDeEscuchaDeMensajes();

	std::string mensaje_a_enviar = "";

	while (mensaje_a_enviar != "SALIR" && !mensaje_salir_recibido)
	{
		std::cout << "Envíe mensaje 'SALIR' para terminar aplicación local y remota." << std::endl;
		std::cout << "Enviar mensaje por Puerto Serie DE 5 CARACTERES: ";

		std::cin >> mensaje_a_enviar;

		std::cout << std::endl;

		puerto->enviarMensaje(mensaje_a_enviar, mensaje_a_enviar.length());
	}

	puerto->finalizarLoopDeEscuchaDeMensajes();
	
	puerto->cerrarConexionPuertoSerie();
	delete puerto;
	return 0;
}