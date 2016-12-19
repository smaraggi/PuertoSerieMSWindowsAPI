#ifndef PUERTOSERIE_H_
#define PUERTOSERIE_H_

#include <Windows.h>

#include <string>
#include <iostream>

// Referencia (API de Windows SDK para puerto serie):
// https://msdn.microsoft.com/en-us/library/ff802693.aspx

/// \brief Clase que modela un puerto serie. La clase est� preparada para administrar el puerto con env�o y recepci�n sincr�nica.
/// El bufer de lectura acumula caracteres que lleguen del puerto serie hasta alcanzar la cantidad especificada en el largo
/// de mensaje que se desea recibir.
class PuertoSerie
{
public:	
	// creador
	// Nombre de puerto: COM1, COM2...
	// Par�metros de puerto y timeouts de puerto
	// largo fijo del mensaje que se espera recibir, debe ser un mismo largo para todos los mensajes entrantes.
	PuertoSerie(std::string nombrePuerto, DCB parametrosPuerto, COMMTIMEOUTS timeoutsPuerto, int largoMensajeARecibir);

	// Destructor
	~PuertoSerie();

	// Abre el puerto serie
	bool abrirConexionPuertoSerie();

	// Cierra la conexi�n del puerto serie
	bool cerrarConexionPuertoSerie();

	// env�a el mensaje contenido en string y pasa la cantidad de caracteres a enviar del mismo
	// t�picamente la cantidad de caracteres es el largo total del mensaje a enviar
	// devuelve la cantidad de caracteres finalmente enviados correctamente. 
	// En caso de error de escritura de vuelve -1
	int enviarMensaje(std::string mensaje, int candidad_caracteres);

	// inicio de loop de escucha de mensajes.
	// Lanza un hilo interno de la clase que queda escuchando la llegada de mensajes
	// ���El hilo queda bloqueado chequeando cada 50ms si no se indic� que debe terminar ???
	void iniciarLoopDeEscuchaDeMensajes();

	// finaliza el loop de escucha de mensajes y destruye el hilo de escucha de mensajes.
	void finalizarLoopDeEscuchaDeMensajes();

	// Setter de funci�n de procesamiento de mensajes
	void setFuncionDeProcesamientoDeMensajes(void (*procesarMensajeRecibido) (std::string));

	// setea o anula se�al DTR. Devuelve true si la operaci�n concluy� con �xito
	bool setDataTerminalReady(bool dtr);

	// Fuerza vaciado de buffers de escritura y lectura terminando las operaciones pendientes
	// En caso de error devuelve cero, se puede consultar mediante GetLastError
	bool completarOperacionesYVaciarBuffers();

private:

	// La instancia del puerto se pasa como void*
	// El puerto debe tener un timeout de escucha, sale de la lectura, consulta si debe concluir y vuelve a escuchar
	static DWORD WINAPI FuncionEscuchaDePuertoSerie(LPVOID handle_puerto); 

	// Funci�n que procesa un mensaje recibido
	void (*funcion_procesar_mensaje) (std::string mensaje);

	// Nombre del puerto ("COM1", "COM2", etc.)
	std::string nombre_puerto;

	// Par�metros puerto serie
	DCB parametros_puerto_serie;

	// Par�metros de timeout de la comunicaci�n
	COMMTIMEOUTS timeouts;

	// HANDLE del puerto comm establecido
	HANDLE handle_puerto_serie;

	// Flag para finalizar escucha de mensajes
	bool finalizarEscucha;

	// Evento para finalizar la escucha de mensajes
	HANDLE handle_evento_fin_escucha_mensajes;

	// HANDLE de hilo de escucha de mensajes
	HANDLE handle_thread_escucha_mensajes;

	// Largo del mensaje que se desea recibir
	int largo_mensaje_a_recibir;

	// ID de thread de escucha de mensajes
	int id_thread_escucha;
};

#endif // PUERTOSERIE_H_