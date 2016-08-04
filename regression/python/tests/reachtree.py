import unittest

class Reachtree(unittest.TestCase):
    def setUp(self):
        import esbmc
        # cwd = regression/python
        self.ns, self.opts, self.po = esbmc.init_esbmc_process(['test_data/00_big_endian_01/main.c', '--big-endian', '--bv'])
        self.funcs = self.po.goto_functions
        self.main = self.funcs.function_map[esbmc.irep_idt('c::main')].body
        self.eq = esbmc.symex.equation(self.ns)
        self.art = esbmc.symex.reachability_tree(self.po.goto_functions, self.ns, self.opts, self.eq, self.po.context, self.po.message_handler)

    def tearDown(self):
        import esbmc
        esbmc.kill_esbmc_process()
        self.ns, self.opts, self.funcs = None, None, None
        self.main = None
        self.eq = None
        self.art = None

    def test_setup(self):
        # Simply check that the call to create a reach-tree worked
        pass

    def test_full(self):
        import esbmc
        # Run a full model checking process (:O)
        self.art.setup_for_new_explore()
        result = self.art.get_next_formula()
        btor = esbmc.solve.solvers.boolector.make(False, False, self.ns, self.opts)
        result.target.convert(btor)
        issat = btor.dec_solve()
        # This test case should have a counterexample
        self.assertTrue(issat == esbmc.solve.smt_result.sat, "Full BMC result should be SAT")

    def test_full_internal(self):
        import esbmc
        # Run a full model checking process (:O)
        self.art.setup_for_new_explore()

            # multithreaded crut necessary for flags being set
        while ((not self.art.get_cur_state().has_cswitch_point_occured() or self.art.get_cur_state().check_if_ileaves_blocked()) and self.art.get_cur_state().can_execution_continue()):
            self.art.get_cur_state().symex_step(self.art)

        # After running to a blocked point, try to context switch, fail,
        # which results in flags being set
        #self.art.create_next_state()
        self.art.switch_to_next_execution_state()
        self.assertTrue(self.art.is_has_complete_formula())

        self.art.get_cur_state().finish_formula()
        self.art.has_complete_formula = False;
        result = get_cur_state().get_symex_result();

        btor = esbmc.solve.solvers.boolector.make(False, False, self.ns, self.opts)
        result.target.convert(btor)
        issat = btor.dec_solve()
        # This test case should have a counterexample
        self.assertTrue(issat == esbmc.solve.smt_result.sat, "Full BMC result should be SAT")
