import re
def convert_file(in_file, out_file):
    with open(in_file, 'r') as f:
        src = f.read()

    src = src.replace('bits 64', 'bits 32')
    src = src.replace('r12', 'ebp')
    src = src.replace('r13', 'esi')
    src = src.replace('r14', 'edi')
    src = src.replace('r15', 'esp') # wait, no this is bad. We can just use standard pushing

    # Let's completely rewrite it manually using sed for registers if we can, or just write a python script that outputs the correct x86 assembler.
