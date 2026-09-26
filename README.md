# Nodex

<img src="docs/nodex.png">

![GCC](https://img.shields.io/badge/GCC-14.2%2B-blue)
![Language](https://img.shields.io/badge/language-C99%2FC11-blue)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20BSD%20%7C%20macOS%20%7C%20Windows-lightgrey)
![Consensus](https://img.shields.io/badge/consensus-IBFT%202.0%20%7C%20QBFT-purple)
![Cryptography](https://img.shields.io/badge/cryptography-ECDSA-orange)
![License](https://img.shields.io/badge/license-GPLv3-green)

**Nodex** es una implementación experimental y modular de una cadena de bloques escrita en **C**, diseñada para explorar redes distribuidas, comunicación **P2P**, consenso tolerante a fallos bizantinos y autenticación criptográfica.

El proyecto utiliza un modelo de consenso inspirado en **IBFT 2.0 / QBFT** y firmas digitales **ECDSA** para autenticar transacciones y bloques.

> **Estado:** 🚧 Experimental / MVP en desarrollo

---

## ✨ Características

* ⛓️ Blockchain modular implementada en C.
* 🌐 Comunicación entre nodos mediante red P2P.
* 🤝 Protocolo de consenso basado en **IBFT 2.0 / QBFT**.
* 🔐 Firmas digitales mediante **ECDSA**.
* 🔑 Generación y administración de pares de claves.
* 📝 Creación y firma de transacciones.
* 📦 Mempool para transacciones pendientes.
* 💾 Persistencia local de la cadena.
* 🔄 Sincronización entre nodos.
* 🖥️ Cliente CLI para administrar e interactuar con un nodo.
* 🧩 Arquitectura modular para facilitar futuras extensiones.

---

## 🏗️ Arquitectura

Nodex está organizado en diferentes módulos responsables de las principales capas del sistema:

```text
                         ┌─────────────────────┐
                         │      node_cli       │
                         │   CLI / Wallet      │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │    node_daemon      │
                         │   Node / Server     │
                         └──────────┬──────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              │                     │                     │
              ▼                     ▼                     ▼
       ┌─────────────┐       ┌─────────────┐       ┌─────────────┐
       │    P2P      │       │  Consensus  │       │   Mempool   │
       │    Layer    │       │ IBFT / QBFT │       │ Transactions │
       └──────┬──────┘       └──────┬──────┘       └─────────────┘
              │                     │
              └──────────┬──────────┘
                         ▼
                 ┌───────────────┐
                 │   Blockchain  │
                 │     / DB      │
                 └───────────────┘
```

### Componentes principales

| Componente    | Responsabilidad                                       |
| ------------- | ----------------------------------------------------- |
| `blockchain`  | Gestión de bloques y cadena                           |
| `consensus`   | Lógica del protocolo de consenso                      |
| `p2p_sync`    | Comunicación y sincronización entre nodos             |
| `transaction` | Creación, validación y procesamiento de transacciones |
| `storage`     | Persistencia de la cadena                             |
| `utils`       | Funciones criptográficas y utilidades                 |
| `node_daemon` | Proceso principal del nodo                            |
| `node_cli`    | Interfaz de línea de comandos                         |

---

## 📁 Estructura del Proyecto

```text
.
├── lib/
│   ├── blockchain.h
│   ├── consensus.h
│   ├── p2p_sync.h
│   ├── storage.h
│   ├── transaction.h
│   └── utils.h
│
├── src/
│   ├── blockchain.c
│   ├── consensus.c
│   ├── node_cli.c
│   ├── node_daemon.c
│   ├── p2p_sync.c
│   └── ...
│
├── data/
│   └── blockchain.json
│
├── Makefile
├── LICENSE
└── README.md
```

---

# 🚀 Instalación

## Requisitos

Para compilar Nodex se necesita un entorno de desarrollo compatible con C y las siguientes herramientas:

* **GCC 14.2+** o Clang
* **Make**
* **OpenSSL**
* Librerías de desarrollo de OpenSSL

### Debian / Ubuntu

```bash
sudo apt update
sudo apt install build-essential libssl-dev
```

### Arch Linux

```bash
sudo pacman -S base-devel openssl
```

### macOS

Con Homebrew:

```bash
brew install gcc openssl make
```

> Los nombres y ubicaciones de las librerías pueden variar dependiendo de la distribución y del sistema operativo.

---

# 📥 Clonar el repositorio

```bash
git clone https://github.com/nerdemma/blockchain.git
cd blockchain
```

---

# 🔨 Compilación

El proyecto utiliza `Makefile` para automatizar la compilación.

```bash
make
```

Después de una compilación exitosa deberían estar disponibles los ejecutables principales:

```text
node_daemon
node_cli
```

Para limpiar los archivos generados:

```bash
make clean
```

---

# 💻 Uso

## 1. Iniciar un nodo

El nodo puede iniciarse especificando el puerto P2P y el archivo de persistencia:

```bash
./node_daemon --port 8080 --db data/blockchain.json
```

El proceso se encarga de:

1. Inicializar el nodo.
2. Cargar la blockchain existente.
3. Crear el bloque génesis si todavía no existe.
4. Inicializar la mempool.
5. Esperar conexiones P2P.
6. Procesar transacciones.
7. Participar en el protocolo de consenso.

---

## 2. Generar un par de claves

Para firmar transacciones se puede generar un par de claves criptográficas:

```bash
./node_cli --gen-keypair --out wallet.key
```

El archivo generado contiene las credenciales necesarias para firmar transacciones.

> ⚠️ **Importante:** las claves privadas deben mantenerse seguras. No deben publicarse en Git ni almacenarse en repositorios públicos.

---

## 3. Crear y firmar una transacción

Una transacción puede ser enviada utilizando la clave privada:

```bash
./node_cli \
    --send-tx \
    --key wallet.key \
    --to <DIRECCION_DESTINO> \
    --amount 50
```

El nodo deberá validar la transacción antes de incorporarla a la mempool.

---

## 4. Consultar el estado del nodo

Para consultar información básica del nodo:

```bash
./node_cli --status
```

Dependiendo de la versión implementada, el comando puede proporcionar información como:

* Altura actual de la blockchain.
* Estado del nodo.
* Cantidad de transacciones pendientes.
* Información de sincronización.

---

# 🌐 Red P2P

Los nodos de Nodex están diseñados para comunicarse entre sí mediante una red **peer-to-peer**.

Conceptualmente:

```text
             ┌───────────────┐
             │     Node A    │
             └───────┬───────┘
                     │
              ┌──────┴──────┐
              │     P2P      │
              │    Network   │
              └───┬──────┬───┘
                  │      │
          ┌───────┘      └───────┐
          ▼                       ▼
   ┌─────────────┐        ┌─────────────┐
   │   Node B    │        │   Node C    │
   └─────────────┘        └─────────────┘
```

La capa P2P es responsable de transportar información relacionada con:

* Transacciones.
* Bloques.
* Mensajes de consenso.
* Información necesaria para sincronización.

---

# 🤝 Consenso

Nodex implementa una arquitectura orientada a un mecanismo de consenso basado en:

**IBFT 2.0 / QBFT**

El objetivo de este modelo es permitir que un conjunto de nodos validadores acuerde el siguiente bloque sin depender de minería basada en Proof of Work.

De forma simplificada:

```text
Transaction
     │
     ▼
   Mempool
     │
     ▼
 Proposer
     │
     ▼
 Validators
     │
     ▼
 Consensus
     │
     ▼
 New Block
     │
     ▼
 Blockchain
```

La implementación se encuentra en desarrollo y está destinada principalmente a fines experimentales, educativos y de investigación.

---

# 🔐 Criptografía

Nodex utiliza **OpenSSL** para las operaciones criptográficas.

Entre los componentes previstos se incluyen:

* ECDSA para firmas digitales.
* SHA-256 para funciones hash.
* Validación de firmas.
* Autenticación de transacciones.
* Integridad de bloques.

La criptografía se utiliza para garantizar que las transacciones puedan ser verificadas mediante sus correspondientes firmas digitales.

---

# 💾 Persistencia

La blockchain puede almacenarse localmente para permitir que un nodo recupere su estado después de reiniciarse.

Ejemplo:

```text
data/
└── blockchain.json
```

La capa de almacenamiento se encuentra separada de la lógica principal de blockchain para permitir futuras implementaciones de diferentes backends.

---

# 🧪 Estado del Proyecto

Nodex se encuentra actualmente en una etapa **experimental / MVP**.

### Implementado

* [x] Estructura modular del proyecto
* [x] Blockchain
* [x] Transacciones
* [x] Mempool
* [x] Persistencia
* [x] CLI
* [x] Nodo daemon
* [x] Comunicación P2P
* [x] Integración criptográfica
* [x] Base del mecanismo de consenso

### En desarrollo

* [ ] Pruebas automatizadas más completas
* [ ] Sincronización avanzada entre nodos
* [ ] Manejo robusto de forks
* [ ] Validación exhaustiva de bloques
* [ ] Implementación completa de las fases IBFT/QBFT
* [ ] Gestión avanzada de validadores
* [ ] API para aplicaciones externas
* [ ] Documentación técnica del protocolo
* [ ] Benchmarks de rendimiento
* [ ] Tests de interoperabilidad entre sistemas operativos

---

# 🗺️ Roadmap

```text
[x] Blockchain básica
[x] Transacciones
[x] Mempool
[x] Persistencia
[x] CLI
[x] P2P básico
[x] Criptografía
[ ] Consenso IBFT/QBFT completo
[ ] Sincronización avanzada
[ ] API
[ ] Tests automatizados
[ ] Benchmarking
[ ] Documentación del protocolo
[ ] Red de prueba
[ ] Herramientas de administración de nodos
```

---

# 🎯 Objetivos

El proyecto busca servir como plataforma experimental para estudiar y desarrollar componentes relacionados con:

* Sistemas distribuidos.
* Redes P2P.
* Consenso bizantino.
* Criptografía aplicada.
* Desarrollo de software de bajo nivel.
* Programación de sistemas en C.
* Arquitecturas blockchain.
* Comunicación entre nodos.

Uno de los objetivos principales es mantener una implementación relativamente pequeña y comprensible, permitiendo estudiar cómo diferentes componentes de una blockchain interactúan entre sí.

---

# ⚠️ Disclaimer

**Nodex es un proyecto experimental.**

No debe utilizarse actualmente para almacenar fondos, activos reales, información sensible o aplicaciones de producción sin una auditoría de seguridad independiente y pruebas adicionales.

El código se proporciona con fines educativos, experimentales y de investigación.

---

# 📜 Licencia

Nodex se distribuye bajo la licencia **GNU General Public License v3.0 (GPLv3)**.

Consulta el archivo [`LICENSE`](LICENSE) para obtener el texto completo de la licencia.

---

# 👨‍💻 Autor

**Emmanuel D. Breyaue**

GitHub: [@nerdemma](https://github.com/nerdemma)

---

## ⭐ Contribuciones

Las contribuciones, correcciones y propuestas de mejora son bienvenidas.

Si encuentras un problema o tienes una propuesta:

1. Abre un **Issue**.
2. Describe el problema o propuesta.
3. Incluye información relevante para reproducirlo.
4. Para cambios de código, abre un **Pull Request**.

---

## 📌 Nota

Nodex está evolucionando como un proyecto de investigación y desarrollo en **C**, con especial interés en la combinación de programación de sistemas, redes P2P, criptografía y mecanismos de consenso distribuido.
