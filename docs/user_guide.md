# 📖 Guía de Usuario - Cliente de Chat WebSockets

Esta guía explica cómo utilizar el cliente de chat basado en WebSockets.

## 📥 Instalación
### 🔧 **Requisitos**
- **C++17 o superior**
- **CMake**
- **Bibliotecas necesarias**: WebSockets (dependiendo de la implementación específica)

### 🚀 **Compilación del Cliente**
```bash
mkdir build && cd build
cmake ..
make
```

### ▶️ **Ejecutar el Cliente**
```bash
./client
```

---

## 📡 Conectarse al Servidor
Cuando el cliente se inicia, debe proporcionar un nombre de usuario para conectarse al servidor WebSockets.
```bash
ws://<host>:<port>?name=<username>
```

⚠ **Restricciones:**
- El nombre de usuario no puede ser vacío ni `~` (reservado para el chat general).
- Si el usuario ya está en línea, la conexión será rechazada.
- Si el usuario existía pero estaba desconectado, se aceptará la conexión.

---

## 💬 Funcionalidades

### **1. Listar Usuarios Conectados**
Para ver qué usuarios están en línea:
```
[1]
```

### **2. Consultar Información de un Usuario**
Para obtener el estado de un usuario específico:
```
[2] [3] ["Ana"]
```
(Consulta el estado del usuario `Ana`)

### **3. Cambiar Estado**
Para actualizar tu estado:
```
[3] [3] ["Ana"] [1]
```
(El usuario `Ana` cambia su estado a `Activo`)

### **4. Enviar un Mensaje**
Para enviar un mensaje a otro usuario:
```
[4] [3] ["Ana"] [5] ["Hola"]
```
(Envía `Hola` a `Ana`)

Para enviar un mensaje al chat general:
```
[4] [1] ["~"] [12] ["Buenos días"]
```

### **5. Obtener Historial de Chat**
Si un usuario quiere recuperar mensajes anteriores:
```
[5] [3] ["Ana"]
```
(Solicita el historial del chat con `Ana`)

---

## ❌ Manejo de Errores
Si algo sale mal, el servidor enviará un mensaje de error con el siguiente formato:

| Código | Descripción |
|--------|------------|
| 1 | Usuario no existe. |
| 2 | Estado inválido. |
| 3 | Mensaje vacío. |
| 4 | Usuario destinatario desconectado. |

---