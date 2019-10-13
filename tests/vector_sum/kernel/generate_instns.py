import numpy as np
from numpy import binary_repr

class InsGen():
    def __init__(self):
        self.shift_Op1 = 24
        self.shift_Op2 = 16
        self.shift_Op3 = 8
        self.shift_sel_signals = 0
        self.total_instns = 10
        self.idx_Op1 = np.zeros(self.total_instns,dtype=int)
        self.idx_Op2 = np.zeros(self.total_instns,dtype=int)
        self.idx_Op3 = np.zeros(self.total_instns,dtype=int)
        self.sel_signals = np.zeros(self.total_instns,dtype=int)

    def generate_op_indices(self):

        it = 0
        sel_sig_adder_is_Op2_self = 0
        for i in range(0,self.total_instns):
            self.idx_Op3[it] = 0
            self.idx_Op2[it] = 0
            self.idx_Op1[it] = i
            update_data_read_and_write_op =  i
            if(update_data_read_and_write_op == 0):
                sel_sig_adder_is_Op2_self = 1
            else:
                sel_sig_adder_is_Op2_self = 0
            self.sel_signals[it] = sel_sig_adder_is_Op2_self
            it += 1

    def print_instructions(self):
        with open("PEInstns.txt", "w") as text_file:
            for it in range(self.total_instns):
                Op1=self.idx_Op1[it] # I
                Op2=self.idx_Op2[it] # W
                Op3=self.idx_Op3[it] # O
                sel_signals=self.sel_signals[it]
                long_instn = (Op3 << self.shift_Op3) + (Op2 << self.shift_Op2) + (Op1 << self.shift_Op1) + (sel_signals << self.shift_sel_signals)
                # bin_instn = binary_repr(long_instn, 24)
                # print(bin_instn)
                # text_file.write(bin_instn+"\n")
                text_file.write(long_instn.astype(str)+"\n")

        with open("PEInstns.bin", "w") as text_file:
            for it in range(self.total_instns):
                Op1=self.idx_Op1[it] # I
                Op2=self.idx_Op2[it] # W
                Op3=self.idx_Op3[it] # O
                sel_signals=self.sel_signals[it]
                long_instn = (Op3 << self.shift_Op3) + (Op2 << self.shift_Op2) + (Op1 << self.shift_Op1) + (sel_signals << self.shift_sel_signals)
                bin_instn = binary_repr(long_instn, 32)
                text_file.write(bin_instn+"\n")

def main():
    codegen = InsGen()
    codegen.generate_op_indices()
    codegen.print_instructions()

if __name__ == "__main__":
    main()
