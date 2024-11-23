CPP=g++
CARGS=-std=c++17 -O3 -g0 -m64
BRKGAINC=-I /home/luishpmendes/UNICAMP/Doutorado/nsbrkga/nsbrkga
BOOSTINC=-I /opt/boost/include -L /opt/boost/lib -lboost_serialization
PAGMOINC=-I /opt/pagmo/include -L /opt/pagmo/lib -Wl,-R/opt/pagmo/lib -lpagmo -ltbb -pthread
INC=-I src $(BRKGAINC) $(BOOSTINC) $(PAGMOINC)
MKDIR=mkdir -p
RM=rm -rf
SRC=$(PWD)/src
BIN=$(PWD)/bin

clean:
	@echo "--> Cleaning compiled..."
	$(RM) $(BIN)
	@echo

$(BIN)/%.o: $(SRC)/%.cpp
	@echo "--> Compiling $<..."
	$(MKDIR) $(@D)
	$(CPP) $(CARGS) -c $< -o $@ $(INC)
	@echo

$(BIN)/test/instance_test : $(BIN)/instance/instance.o \
                            $(BIN)/test/instance_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/instance_test
	@echo

instance_test : $(BIN)/test/instance_test

$(BIN)/test/solution_test : $(BIN)/instance/instance.o \
                            $(BIN)/solution/solution.o \
                            $(BIN)/test/solution_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/solution_test
	@echo

solution_test : $(BIN)/test/solution_test

$(BIN)/test/nsga2_solver_test : $(BIN)/instance/instance.o \
                                $(BIN)/solution/solution.o \
                                $(BIN)/solver/solver.o \
                                $(BIN)/solver/nsga2/problem.o \
                                $(BIN)/solver/nsga2/nsga2_solver.o \
                                $(BIN)/test/nsga2_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/nsga2_solver_test
	@echo

nsga2_solver_test : $(BIN)/test/nsga2_solver_test

$(BIN)/test/nspso_solver_test : $(BIN)/instance/instance.o \
                                $(BIN)/solution/solution.o \
                                $(BIN)/solver/solver.o \
                                $(BIN)/solver/nspso/problem.o \
                                $(BIN)/solver/nspso/nspso_solver.o \
                                $(BIN)/test/nspso_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/nspso_solver_test
	@echo

nspso_solver_test : $(BIN)/test/nspso_solver_test

tests : instance_test \
				solution_test \
				nsga2_solver_test \
				nspso_solver_test

all : tests
