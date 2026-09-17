#!/bin/bash
set -euo pipefail

# Colores para la salida
GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Configuración
CLI="../node_cli"
PEER_TARGET="192.168.100.189:8333"
VALIDATOR_ADDR="VAL_NODE_01"

# Verificar que el ejecutable existe
if [ ! -f "$CLI" ]; then
    echo -e "${RED}[!] Error: No se encontró el ejecutable $CLI. Compilá primero con 'make'.${NC}"
    exit 1
fi

echo -e "${CYAN}=== INICIANDO SUITE DE PRUEBAS DEL NODO ===${NC}\n"

echo -e "${GREEN}[1] Estado inicial de la blockchain y balances:${NC}"
$CLI --status
echo "----------------------------------------"
$CLI --balance "ALICE"
$CLI --balance "BOB"

echo -e "\n${GREEN}[2] Inyectando transacciones de prueba a la mempool:${NC}"
$CLI --tx "ALICE:BOB:50.0"
$CLI --tx "BOB:CARLIE:12.5"

echo -e "\n${GREEN}[3] Proponiendo un nuevo bloque (Consenso PoA):${NC}"
$CLI --propose "$VALIDATOR_ADDR"

echo -e "\n${GREEN}[4] Verificando el estado de la cadena y balances actualizados:${NC}"
$CLI --status
echo "----------------------------------------"
$CLI --balance "ALICE"
$CLI --balance "BOB"
$CLI --balance "CARLIE"

echo -e "\n${GREEN}[5] Probando conectividad P2P hacia el peer remoto:${NC}"
$CLI --connect "$PEER_TARGET"

echo -e "\n${CYAN}=== PRUEBAS COMPLETADAS ===${NC}"
