#!/bin/bash
set -euo pipefail

# Colores para la salida
GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuración
CLI="../node_cli"
PEER_TARGET="192.168.100.120:8333"
VALIDATOR_ADDR="VAL_NODE_01"

# Verificar que el ejecutable existe
if [ ! -f "$CLI" ]; then
    echo -e "${RED}[!] Error: No se encontró el ejecutable $CLI. Compilá primero con 'make'.${NC}"
    exit 1
fi

echo -e "${CYAN}=== INICIANDO SUITE DE PRUEBAS DEL NODO ===${NC}\n"

echo -e "${GREEN}[1] Estado inicial de la blockchain y balances (JSON):${NC}"
$CLI --status-json || { echo -e "${RED}Error: --status-json falló${NC}"; exit 1; }
echo "----------------------------------------"
$CLI --balance-json "ALICE" || true
$CLI --balance-json "BOB" || true

echo -e "\n${GREEN}[2] Inyectando transacciones de prueba a la mempool (firmadas):${NC}"
# Generar clave ECDSA secp256k1 temporal para pruebas
openssl ecparam -name secp256k1 -genkey -noout -out /tmp/test_key.pem
PRIV_HEX=$(openssl ec -in /tmp/test_key.pem -text -noout | awk '/priv:/{f=1;next}/pub:/{f=0}f{gsub(/ /,"");printf $0}'; echo)
echo "Usando clave privada (hex): ${PRIV_HEX}"

# Enviar transacciones firmadas con la clave generada
$CLI --key "$PRIV_HEX" --tx "ALICE:BOB:50.0"
$CLI --key "$PRIV_HEX" --tx "BOB:CARLIE:12.5"

# Verificar mempool JSON y que las transacciones incluyen signature
$CLI --mempool-json > /tmp/mempool.json || { echo -e "${RED}Error: --mempool-json falló${NC}"; exit 1; }
if ! grep -q "\"signature\"" /tmp/mempool.json; then
    echo -e "${RED}[!] Error: mempool JSON no contiene firmas.${NC}"
    cat /tmp/mempool.json
    exit 2
fi

# Verificar persistencia atómica: archivo mempool y permisos
if [ -f "mempool.dat" ]; then
    perms=$(stat -c %a mempool.dat)
    echo "mempool.dat permisos: $perms"
else
    echo -e "${RED}[!] Error: mempool.dat no existe.${NC}"; exit 3
fi

echo -e "\n${GREEN}[3] Proponiendo un nuevo bloque (Consenso PoA):${NC}"
$CLI --propose "$VALIDATOR_ADDR"

echo -e "\n${GREEN}[4] Verificando el estado de la cadena y balances actualizados (JSON):${NC}"
$CLI --status-json || true
echo "----------------------------------------"
$CLI --balance-json "ALICE" || true
$CLI --balance-json "BOB" || true
$CLI --balance-json "CARLIE" || true

echo -e "\n${GREEN}[5] Probando conectividad P2P hacia el peer remoto:${NC}"
$CLI --connect "$PEER_TARGET"

echo -e "\n${CYAN}=== PRUEBAS COMPLETADAS ===${NC}"
