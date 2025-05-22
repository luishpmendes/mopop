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

$(BIN)/test/moead_solver_test : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/moead/problem.o \
																$(BIN)/solver/moead/moead_solver.o \
																$(BIN)/test/moead_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/moead_solver_test
	@echo

moead_solver_test : $(BIN)/test/moead_solver_test

$(BIN)/test/mhaco_solver_test : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/mhaco/problem.o \
																$(BIN)/solver/mhaco/mhaco_solver.o \
																$(BIN)/test/mhaco_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/mhaco_solver_test
	@echo

mhaco_solver_test : $(BIN)/test/mhaco_solver_test

$(BIN)/test/ihs_solver_test : $(BIN)/instance/instance.o \
															$(BIN)/solution/solution.o \
															$(BIN)/solver/solver.o \
															$(BIN)/solver/ihs/problem.o \
															$(BIN)/solver/ihs/ihs_solver.o \
															$(BIN)/test/ihs_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/ihs_solver_test
	@echo

ihs_solver_test : $(BIN)/test/ihs_solver_test

$(BIN)/test/nsbrkga_solver_test : $(BIN)/instance/instance.o \
																	$(BIN)/solution/solution.o \
																	$(BIN)/solver/solver.o \
																	$(BIN)/solver/nsbrkga/decoder.o \
																	$(BIN)/solver/nsbrkga/nsbrkga_solver.o \
																	$(BIN)/test/nsbrkga_solver_test.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo
	@echo "--> Running test..."
	$(BIN)/test/nsbrkga_solver_test
	@echo

nsbrkga_solver_test : $(BIN)/test/nsbrkga_solver_test

$(BIN)/exec/nsga2_solver_exec : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/nsga2/problem.o \
																$(BIN)/solver/nsga2/nsga2_solver.o \
																$(BIN)/utils/argument_parser.o \
																$(BIN)/exec/nsga2_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

nsga2_solver_exec : $(BIN)/exec/nsga2_solver_exec

$(BIN)/exec/nspso_solver_exec : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/nspso/problem.o \
																$(BIN)/solver/nspso/nspso_solver.o \
																$(BIN)/utils/argument_parser.o \
																$(BIN)/exec/nspso_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

nspso_solver_exec : $(BIN)/exec/nspso_solver_exec

$(BIN)/exec/moead_solver_exec : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/moead/problem.o \
																$(BIN)/solver/moead/moead_solver.o \
																$(BIN)/utils/argument_parser.o \
																$(BIN)/exec/moead_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

moead_solver_exec : $(BIN)/exec/moead_solver_exec

$(BIN)/exec/mhaco_solver_exec : $(BIN)/instance/instance.o \
																$(BIN)/solution/solution.o \
																$(BIN)/solver/solver.o \
																$(BIN)/solver/mhaco/problem.o \
																$(BIN)/solver/mhaco/mhaco_solver.o \
																$(BIN)/utils/argument_parser.o \
																$(BIN)/exec/mhaco_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

mhaco_solver_exec : $(BIN)/exec/mhaco_solver_exec

$(BIN)/exec/ihs_solver_exec : $(BIN)/instance/instance.o \
															$(BIN)/solution/solution.o \
															$(BIN)/solver/solver.o \
															$(BIN)/solver/ihs/problem.o \
															$(BIN)/solver/ihs/ihs_solver.o \
															$(BIN)/utils/argument_parser.o \
															$(BIN)/exec/ihs_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

ihs_solver_exec : $(BIN)/exec/ihs_solver_exec

$(BIN)/exec/nsbrkga_solver_exec : $(BIN)/instance/instance.o \
																	$(BIN)/solution/solution.o \
																	$(BIN)/solver/solver.o \
																	$(BIN)/solver/nsbrkga/decoder.o \
																	$(BIN)/solver/nsbrkga/nsbrkga_solver.o \
																	$(BIN)/utils/argument_parser.o \
																	$(BIN)/exec/nsbrkga_solver_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

nsbrkga_solver_exec : $(BIN)/exec/nsbrkga_solver_exec

$(BIN)/exec/reference_pareto_front_and_point_calculator_exec : $(BIN)/instance/instance.o \
																															 $(BIN)/solution/solution.o \
																															 $(BIN)/solver/solver.o \
																															 $(BIN)/utils/argument_parser.o \
																															 $(BIN)/exec/reference_pareto_front_and_point_calculator_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

reference_pareto_front_and_point_calculator_exec : $(BIN)/exec/reference_pareto_front_and_point_calculator_exec

$(BIN)/exec/hypervolume_calculator_exec : $(BIN)/instance/instance.o \
																					$(BIN)/utils/argument_parser.o \
																					$(BIN)/exec/hypervolume_calculator_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

hypervolume_calculator_exec : $(BIN)/exec/hypervolume_calculator_exec

$(BIN)/exec/modified_generational_distance_calculator_exec : $(BIN)/instance/instance.o \
																														 $(BIN)/utils/argument_parser.o \
																														 $(BIN)/exec/modified_generational_distance_calculator_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

modified_generational_distance_calculator_exec : $(BIN)/exec/modified_generational_distance_calculator_exec

$(BIN)/exec/results_aggregator_exec : $(BIN)/utils/argument_parser.o \
																			$(BIN)/exec/results_aggregator_exec.o
	@echo "--> Linking objects..."
	$(CPP) -o $@ $^ $(CARGS) $(INC)
	@echo

results_aggregator_exec : $(BIN)/exec/results_aggregator_exec

tests : instance_test \
				solution_test \
				nsga2_solver_test \
				nspso_solver_test \
				moead_solver_test \
				mhaco_solver_test \
				ihs_solver_test \
				nsbrkga_solver_test

execs : nsga2_solver_exec \
				nspso_solver_exec \
				moead_solver_exec \
				mhaco_solver_exec \
				ihs_solver_exec \
				nsbrkga_solver_exec \
				reference_pareto_front_and_point_calculator_exec \
				hypervolume_calculator_exec \
				modified_generational_distance_calculator_exec \
				results_aggregator_exec

all : tests execs
