# 💬 Cliente de Chat WebSockets

Este es el cliente del sistema de chat basado en WebSockets. Permite a los usuarios conectarse al servidor, enviar y recibir mensajes, y gestionar su estado en la plataforma.

## 🚀 Características
- Conexión a servidor WebSockets
- Envío y recepción de mensajes en tiempo real
- Gestión de estados de usuario
- Historial de mensajes
- Interfaz de usuario básica

## 📂 Estructura del Proyecto
```
client/
│── src/
│   │── main.cpp                 # Punto de entrada del cliente
│
│── include/                       # Archivos de cabecera
│── tests/                         # Pruebas unitarias
│── config/                        # Configuraciones del cliente
│── assets/                        # Recursos estáticos (íconos, imágenes, etc.)
│── docs/                          # Documentación
│── scripts/                       # Scripts auxiliares
│── .gitignore                      # Archivos ignorados por Git
│── README.md                       # Documentación del cliente
```

## 🛠️ Instalación y Ejecución
### 🔧 **Requisitos**
- **C++17 o superior**
- **CMake**
- **Bibliotecas necesarias**: WebSockets (dependiendo de la implementación específica)

### 🚀 **Compilación**
```bash
mkdir build && cd build
cmake ..
make
```

### ▶️ **Ejecutar el cliente**
```bash
./client
```

## 🔄 Comunicación con el Servidor
El cliente envía y recibe mensajes con el siguiente formato:

| Código | Acción | Campos |
|--------|--------|--------|
| 1 | Listar usuarios | - |
| 2 | Obtener usuario | Nombre |
| 3 | Cambiar estado | Nombre, Estado |
| 4 | Enviar mensaje | Destino, Mensaje |
| 5 | Obtener historial | Chat |

## ⚠️ Manejo de Errores
El servidor puede responder con los siguientes errores:
| Código | Descripción |
|--------|------------|
| 1 | Usuario no existe |
| 2 | Estado inválido |
| 3 | Mensaje vacío |
| 4 | Usuario destinatario desconectado |
