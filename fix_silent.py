import re
with open('src/main.c', 'r') as f:
    data = f.read()

data = re.sub(r'if \(woody->size < sizeof\(Elf32_Ehdr\)\)\s*return \(-1\);', r'if (woody->size < sizeof(Elf32_Ehdr)) { fprintf(stderr, ERR_NOT_ELF); return (-1); }', data)
data = re.sub(r'if \(woody->size < sizeof\(Elf64_Ehdr\)\)\s*return \(-1\);', r'if (woody->size < sizeof(Elf64_Ehdr)) { fprintf(stderr, ERR_NOT_ELF); return (-1); }', data)

with open('src/main.c', 'w') as f:
    f.write(data)
