filename=$1
name=${filename%.*}
echo Assembling...
./Assembler $name.asm
echo Translating...
./translator $name.bin