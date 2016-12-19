@author Santiago Maraggi

Aplicación para control de Puerto Serie implementado sobre API de Microsoft Windows.

Antigias aplicaciones utilizaban ActiveX Controls provistos por Microsoft Windows para la resolución de comunicaciones de Puerto Serie.

Microsoft discontinuó los servicios de ActiveX Controls por lo que los programas que los utilizaban son ejecutables hasta versiones de Windows XP. A partir de Windows Vista las aplicaciones que implementaban ActiveX Controls perdieron todas las funcionalidades asociadas a los mismos y su comportamiento tiende a ser errático.

Los controles ActiveX Control para puerto serie resolvían de manera transparente el procesamiento en hilos y la concurrencia sobre el puerto serie, mientras que la API de Windows para ello mismo requiere tener en cuenta el uso concurrente de recursos y el procesamiento mediante eventos.

PuertoSerie
***********

Se generó un módulo de comunicaciones sobre puerto serie parametrizable (apto para diferentes usos y protocolos) con la API de Windows SDK.

Este módulo trabaja de manera asincrónica con los puertos, tanto para el envío como para la recepción de mensajes, de manera de sortear interbloqueos de operaciones concurrentes sobre secciones críticas en lecturas y escrituras simultáneas.

El proyecto "PuertoSerie" incluye la clase "PuertoSerie" y un módulo de testing para probar su funcionamiento entre dos PC conectadas corriendo el mismo programa.

Se han corrido distintas pruebas siendo el comportamiento el esperado.

Se almacenan caracteres hasta completar una palabra según el valor especificado (5 caracteres por palabra). Completado el bufer de salida se envía el mensaje.

Al recibir también se acumulan caracteres hasta completar una palabra (5 caracteres).

El módulo puede utilizarse con protocolos de mensajes de longitud variable o de longitud fija, especificando cada cuántos caracteres la aplicación desea procesar un mensaje.

Para protocolos de mensaje de longitud variable la aplicación debe indicar que se le notifique a la llegada de cada caracter. De esta manera a la aplicación este módulo de puerto serie notificará la llegada de cada caracter para que aquella determine cada cuántos caracteres debe ejecutar algún método (protocolo de mensajes de longitud variable).
