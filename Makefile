# ============================================================================
#  🌲 WOODY WOODPACKER - MAKEFILE EXPLOSIVO (125% EDITION) 🌲
# ============================================================================
#  Herramienta de Empaquetado ELF x86_64, Inyección PIE y Cifrado RC4
#  Proyecto de Ciberseguridad / Virus - Escuela 42
# ============================================================================

# ==================== COLORES ANSI ==========================================
NEGRITA     := \033[1m
RESET       := \033[0m
CIAN        := \033[36m
VERDE       := \033[32m
AMARILLO    := \033[33m
ROJO        := \033[31m
AZUL        := \033[34m
MAGENTA     := \033[35m

# ==================== VARIABLES Y RUTAS =====================================
NAME        := woody_woodpacker
CC          := gcc
CFLAGS      := -Wall -Wextra -Werror -I./include

NASM        := nasm
NASMFLAGS   := -f bin

SRC_DIR     := src
OBJ_DIR     := obj
INC_DIR     := include
ASM_DIR     := asm

SRC         := $(wildcard $(SRC_DIR)/*.c)
OBJ         := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

STUB        := $(ASM_DIR)/payload.bin

# ==================== REGLAS PRINCIPALES ====================================

.DEFAULT_GOAL := help
.PHONY: all clean fclean re help test eval header footer

all: header $(STUB) $(NAME) footer

header:
	@echo "$(CIAN)$(NEGRITA)╔══════════════════════════════════════════════════════════════════════╗$(RESET)"
	@echo "$(CIAN)$(NEGRITA)║ $(VERDE)🌲 WOODY WOODPACKER BUILD SYSTEM - Iniciando Proceso de Ensamblaje   $(CIAN)║$(RESET)"
	@echo "$(CIAN)$(NEGRITA)╚══════════════════════════════════════════════════════════════════════╝$(RESET)"
	@echo ""

$(OBJ_DIR):
	@echo "$(AMARILLO)[*] Creando directorio para archivos objeto: $(OBJ_DIR)...$(RESET)"
	@mkdir -p $(OBJ_DIR)

# Regla de compilación de Assembly puro (Generación del binario parásito)
$(STUB): $(ASM_DIR)/payload.s
	@echo "$(MAGENTA)[+] Forjando el Parásito en Ensamblador (KSA + PRGA PIE Payload)...$(RESET)"
	@$(NASM) $(NASMFLAGS) $< -o $@
	@echo "$(VERDE)  ✓ Payload binario generado en $@$(RESET)"

# Compilación de los archivos C a Objeto (.o)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "$(AZUL)[Compiler]$(RESET) Traduciendo $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Enlace (Linking) final del binario ejecutable
$(NAME): $(OBJ)
	@echo "$(AMARILLO)[*] Enlazando todos los módulos C (Packer, ELF Parser, RC4 Crypto)...$(RESET)"
	@$(CC) $(CFLAGS) -o $@ $^
	@echo "$(VERDE)$(NEGRITA)  ✓ ¡Motor Inyector compilado con éxito: ./$(NAME)!$(RESET)\n"

footer:
	@echo "$(CIAN)$(NEGRITA)========================================================================$(RESET)"
	@echo "$(CIAN)🔧 COMPILACIÓN COMPLETADA. Para ver la ayuda escribe: $(AMARILLO)make help$(RESET)"
	@echo "$(CIAN)$(NEGRITA)========================================================================$(RESET)"

clean:
	@echo "$(ROJO)[-] Purgando la basura binaria temporal (.o) y el Payload crudo...$(RESET)"
	@rm -rf $(OBJ_DIR)
	@rm -f $(STUB)

fclean: clean
	@echo "$(ROJO)[-] Exterminando los ejecutables finales y logs de tests...$(RESET)"
	@rm -f $(NAME)
	@rm -f woody
	@rm -f *.log *.txt *.tmp

re: fclean all

# ==================== UTILIDADES BDD (TEST & AYUDA) =========================

test: all
	@echo "$(AMARILLO)🚀 Lanzaando la batería estricta de test automáticos...$(RESET)"
	@./test_auto.sh

eval: all
	@echo "$(AMARILLO)🛡️  Iniciando evaluación interactiva de woody_woodpacker...$(RESET)"
	@./test_eval.sh

help:
	@echo "$(AZUL)$(NEGRITA)========================================================================$(RESET)"
	@echo "$(AZUL)$(NEGRITA)                 🛡️  WOODY WOODPACKER - AYUDA DE MAKE 🛡️                  $(RESET)"
	@echo "$(AZUL)$(NEGRITA)========================================================================$(RESET)"
	@echo "$(VERDE)Comandos Disponibles:$(RESET)"
	@echo "  $(CIAN)make help$(RESET)        - Muestra este menú de ayuda (comportamiento por defecto)."
	@echo "  $(CIAN)make all$(RESET)         - Compila todo el proyecto en modo estricto."
	@echo "  $(CIAN)make clean$(RESET)       - Elimina archivos obj/ y payload.bin crudos."
	@echo "  $(CIAN)make fclean$(RESET)      - Ejecuta clean + borra woody_woodpacker y clones."
	@echo "  $(CIAN)make re$(RESET)          - Recompila el proyecto."
	@echo "  $(CIAN)make test$(RESET)        - Ejecuta test_auto.sh con batería exhaustiva."
	@echo "  $(CIAN)make eval$(RESET)        - Ejecuta test_eval.sh en modo evaluación guiada."
	@echo ""
	@echo "$(AMARILLO)Modo de Uso:$(RESET)"
	@echo "  ./woody_woodpacker <binary>                  $(CIAN)(Modo Random RC4 128-bit)$(RESET)"
	@echo "  ./woody_woodpacker <binary> <hex_32_chars>   $(MAGENTA)(Modo Llave Paramétrica Bonus)$(RESET)"
	@echo ""