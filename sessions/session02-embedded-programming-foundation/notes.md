# Exercises
## 1. Linker Script & Startup code
### 1.1. Start OpenOCD
```bash
openocd -f board/ti_ek-tm4c123gxl.cfg
```

### 1.2. Start GDB
```bash
gdb-multiarch main.elf
```

### 1.3. Debug Workflow (GDB)
```bash
     target remote :3333
     monitor reset halt
     load
     break main
     continue
     print reset_count

     step 
     x/2x 
     p &reset_count
```
### 1.4. Flash Firmware
```bash
    lm4flash file.bin
```