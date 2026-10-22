# Esteganografía por enmascaramiento con eco — construcción (R4)
#
#   make          crea/actualiza el ambiente conda si hace falta y compila cpp/build/echo_hiding
#   make env      solo crea/actualiza el ambiente conda desde environment.yml
#   make clean    borra cpp/build/
#
# No hace falta activar el ambiente: se usa directamente el g++ del ambiente, que ya
# busca headers y librerías en él y agrega el rpath a su lib/ al enlazar.

ENV   := pds-proyecto1
CONDA ?= $(or $(CONDA_EXE),conda)

# Prefijo del ambiente; si aún no existe, la ruta donde `conda env create` lo creará.
ENV_PREFIX := $(shell $(CONDA) env list 2>/dev/null | awk '$$1 == "$(ENV)" {print $$NF}')
ENV_PREFIX := $(or $(ENV_PREFIX),$(shell $(CONDA) info --base 2>/dev/null)/envs/$(ENV))

CXX      := $(ENV_PREFIX)/bin/g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS += -Icpp/include -MMD -MP

BUILD := cpp/build
BIN   := $(BUILD)/echo_hiding
SRC   := cpp/main.cpp $(wildcard cpp/src/*.cpp)
OBJ   := $(patsubst cpp/%.cpp,$(BUILD)/%.o,$(SRC))
STAMP := $(BUILD)/.env-stamp

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD)/%.o: cpp/%.cpp $(STAMP)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Re-sync the environment only when environment.yml changes.
$(STAMP): environment.yml
	@if [ -x "$(ENV_PREFIX)/bin/python" ]; then \
	    $(CONDA) env update -n $(ENV) -f $< --prune; \
	else \
	    $(CONDA) env create -n $(ENV) -f $<; \
	fi
	@mkdir -p $(@D)
	@touch $@

env: $(STAMP)

clean:
	rm -rf $(BUILD)

-include $(OBJ:.o=.d)

.PHONY: all env clean
