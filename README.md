# Chat OS - Websockets

## Descripción General

Esta aplicación es un cliente de chat desarrollado con Qt que permite a los usuarios comunicarse en tiempo real mediante comunicaciones WebSocket. El cliente soporta tanto mensajes generales (broadcast) como conversaciones privadas, y ofrece funcionalidades como estados de usuario, historial de chat y notificaciones del sistema.



## Arquitectura del Sistema

El cliente se compone de los siguientes elementos arquitectónicos:

### Diagrama de Componentes

```
+---------------------+   +----------------------+   +------------------------+
|  Interfaz de Usuario|   |  Controladores       |   |  Comunicación          |
|---------------------|   |----------------------|   |------------------------|
|- MainWindow         |<->|- WebSocketClient     |<->|- WebSocket (QWebSocket)|
|- ConnectionDialog   |   |- MessageBubble       |   |- HTTP (QNetworkManager)|
|- UserChatItem       |   |                      |   |                        |
+---------------------+   +----------------------+   +------------------------+
```

### Patrón de Diseño

El sistema sigue un patrón de diseño Modelo-Vista-Controlador (MVC):
- **Vista**: MainWindow, ConnectionDialog, UserChatItem, MessageBubble
- **Controlador**: WebSocketClient
- **Modelo**: Representado por las estructuras de datos intercambiadas con el servidor

## Componentes Principales

### 1. MainWindow

Clase principal que gestiona la interfaz de usuario y coordina las interacciones con otros componentes. Responsabilidades:
- Gestionar la interfaz de usuario completa
- Mostrar mensajes y actualizaciones de estado
- Manejar eventos de usuario (envío de mensajes, cambios de estado)
- Coordinar la conexión con el servidor

### 2. WebSocketClient

Maneja toda la comunicación WebSocket con el servidor, interpretando los mensajes binarios según el protocolo establecido. Responsabilidades:
- Establecer y mantener la conexión WebSocket
- Serializar y deserializar mensajes binarios
- Emitir señales con información recibida del servidor

### 3. ConnectionDialog

Diálogo para establecer la conexión inicial al servidor, solicitando:
- Nombre de usuario
- Dirección del servidor
- Puerto

### 4. UserChatItem

Widget personalizado para representar usuarios en la lista de contactos, mostrando:
- Avatar generado a partir del nombre
- Nombre de usuario
- Estado actual
- Último mensaje (si existe)

### 5. MessageBubble

Widget para representar mensajes en el área de chat con distintos estilos según su tipo:
- Mensajes enviados (verde)
- Mensajes recibidos (blanco)
- Mensajes del sistema (azul claro)

## Flujo de Comunicación

### Proceso de Conexión

1. El usuario introduce sus credenciales en el ConnectionDialog
2. La aplicación realiza una validación HTTP inicial
3. Al recibir respuesta positiva, establece la conexión WebSocket
4. Solicita la lista de usuarios y el estado actual
5. Notifica a todos los usuarios sobre la nueva conexión

```
+--------+                +--------+               +---------+
| Cliente |                | HTTP   |               | WebSocket|
+--------+                +--------+               +---------+
    |                         |                         |
    |--- Validación HTTP ---->|                         |
    |<-- Respuesta HTTP ------|                         |
    |                         |                         |
    |------------------- Conexión WebSocket ----------->|
    |<---------------- Confirmación --------------------|
    |                                                   |
    |--------------- Solicitud Lista Usuarios --------->|
    |<-------------- Lista Usuarios --------------------|
    |                                                   |
    |--------------- Solicitud Estado Usuario --------->|
    |<-------------- Estado Usuario --------------------|
```

### Envío de Mensajes

1. El usuario selecciona un destinatario (otro usuario o general)
2. Escribe un mensaje y presiona enviar
3. El cliente serializa el mensaje y lo envía al servidor
4. El servidor distribuye el mensaje al destinatario
5. La interfaz muestra el mensaje en la ventana de chat correspondiente

## Protocolo de Comunicación

El cliente implementa un protocolo binario para comunicarse con el servidor WebSocket:

### Mensajes del Cliente al Servidor

| Opcode | Descripción | Formato del payload |
|--------|-------------|---------------------|
| 0x01   | Solicitar lista de usuarios | (sin datos adicionales) |
| 0x02   | Solicitar información de usuario | <uint8:user_len><char[]:username> |
| 0x03   | Cambiar estado de usuario | <uint8:user_len><char[]:username><uint8:status> |
| 0x04   | Enviar mensaje | <uint8:recipient_len><char[]:recipient><uint8:msg_len><char[]:message> |
| 0x05   | Solicitar historial de chat | <uint8:chat_len><char[]:chat_name> |

### Mensajes del Servidor al Cliente

| Opcode | Descripción | Formato del payload |
|--------|-------------|---------------------|
| 0x50   | Error | <uint8:error_code> |
| 0x51   | Lista de usuarios | <uint8:num_users>[<uint8:user_len><char[]:username><uint8:status>]* |
| 0x52   | Información de usuario | <uint8:user_len><char[]:username><uint8:status> |
| 0x53   | Usuario conectado | <uint8:user_len><char[]:username> |
| 0x54   | Cambio de estado de usuario | <uint8:user_len><char[]:username><uint8:new_status> |
| 0x55   | Mensaje recibido | <uint8:sender_len><char[]:sender><uint8:msg_len><char[]:message> |
| 0x56   | Historial de chat | <uint8:num_msgs>[<uint8:sender_len><char[]:sender><uint8:msg_len><char[]:message>]* |

### Estados de Usuario

| Valor | Descripción |
|-------|-------------|
| 0x00  | Desconectado |
| 0x01  | Activo |
| 0x02  | Ocupado |
| 0x03  | Inactivo |

## Características Implementadas

- **Autenticación**: Validación HTTP previa a la conexión WebSocket
- **Chat general**: Mensajes broadcast a todos los usuarios
- **Chat privado**: Conversaciones directas entre dos usuarios
- **Estados de usuario**: Activo, Ocupado, Inactivo
- **Historial de chat**: Recuperación de mensajes previos
- **Notificaciones**: Alertas del sistema sobre conexiones y cambios de estado
- **Interfaz adaptativa**: Paneles redimensionables y responsive
- **Autodetección de IP**: Muestra la IP local del usuario
- **Timer de inactividad**: Cambia automáticamente el estado según la actividad
- **Visualización de información**: Panel con datos del usuario seleccionado

## Gestión de Mensajes

La aplicación gestiona diferentes tipos de mensajes:

1. **Mensajes del sistema**: Notificaciones sobre conexiones, desconexiones y cambios de estado
2. **Mensajes enviados**: Mensajes propios enviados a otros usuarios
3. **Mensajes recibidos**: Mensajes de otros usuarios
4. **Mensajes de historial**: Mensajes previos recuperados del servidor

El sistema distingue entre mensajes regulares y mensajes de historial mediante la bandera `isHistory`. Esto es crucial para evitar duplicados y mostrar correctamente el historial cuando se cambia entre chats.

## Guía de Uso

### Conexión al Servidor

1. Inicie la aplicación
2. Vaya a `Archivo` > `Conectar`
3. Ingrese:
   - Nombre de usuario
   - Dirección del servidor (por defecto: 18.224.60.241)
   - Puerto (por defecto: 18080)
4. Haga clic en "Conectar"

### Enviar Mensajes Generales

1. Seleccione "General Chat" en la pestaña "Broadcast"
2. Escriba su mensaje en el campo de texto inferior
3. Presione el botón de envío o pulse Enter

### Enviar Mensajes Privados

1. Seleccione un usuario en la pestaña "Direct"
2. Escriba su mensaje en el campo de texto inferior
3. Presione el botón de envío o pulse Enter

### Cambiar su Estado

Utilice el menú desplegable en la parte superior izquierda para cambiar entre:
- ACTIVO
- OCUPADO
- INACTIVO

### Ver Información de Usuario

1. Seleccione un usuario
2. Haga clic en el botón de información (ℹ️) en la esquina superior derecha

## Solución de Problemas Comunes

### Error de Conexión HTTP

Si recibe un error durante la validación HTTP:
- Verifique que el servidor esté en línea
- Compruebe que la dirección y puerto sean correctos
- Asegúrese de que el nombre de usuario no esté en uso

### Mensaje "El nombre ya está en uso"

Este error ocurre cuando:
- El nombre de usuario ya está siendo utilizado por otro cliente
- El servidor aún tiene una sesión activa con ese nombre

Solución: Elija un nombre de usuario diferente o espere unos minutos.

### Mensajes que no aparecen correctamente

Si los mensajes aparecen en la ventana incorrecta:
- Cambie a otra ventana de chat y vuelva
- Desconéctese y vuelva a conectarse
- Verifique que la aplicación esté actualizada a la última versión

## Requisitos y Dependencias

- Qt 6.8.3 con módulos:
  - Core
  - GUI
  - Network
  - WebSockets
  - Widgets
- C++17 o superior

## Compilación e Instalación

### Usando Qt Creator

1. Abra el archivo `.pro` en Qt Creator
2. Configure el kit de compilación deseado
3. Haga clic en "Compilar" y luego en "Ejecutar"

### Usando la línea de comandos

```bash
# Crear directorio de compilación
mkdir build && cd build

# Configurar con qmake
qmake ../Client-OS-P1/Client-OS-P1.pro

# Compilar
make

# Ejecutar
./Client-OS-P1
```

## Seguridad

- La aplicación utiliza validación HTTP antes de establecer la conexión WebSocket
- Las contraseñas no se implementan en esta versión
- Los mensajes se transmiten sin cifrado, no es adecuado para información sensible

## Notas de Implementación

- El cliente incluye detección de duplicados para evitar mensajes repetidos
- La inactividad se detecta automáticamente después de 5 minutos (300000 ms)
- Los avatares se generan a partir del primer carácter del nombre de usuario con un color único
- La aplicación gestiona automáticamente la reconexión y actualización de la lista de usuarios
