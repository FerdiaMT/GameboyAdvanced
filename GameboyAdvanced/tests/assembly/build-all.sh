#!/usr/bin/env bash

set -euo pipefail

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir="$script_dir/build"
clang_bin=${CLANG:-clang}
objcopy_bin=${OBJCOPY:-objcopy}

if ! command -v "$clang_bin" >/dev/null 2>&1; then
    echo "Assembler not found: $clang_bin" >&2
    exit 1
fi

if ! command -v readelf >/dev/null 2>&1; then
    echo "ELF inspector not found: readelf" >&2
    exit 1
fi

mkdir -p "$output_dir"

shopt -s nullglob
source_files=("$script_dir"/*.s "$script_dir"/*.c)

if [ "${#source_files[@]}" -eq 0 ]; then
    echo "No assembly or C tests found in $script_dir" >&2
    exit 1
fi

extract_text_section() {
    local object_file=$1
    local binary_file=$2

    # A cross-target objcopy is preferred. Host objcopy installations often do
    # not recognise ARM ELF files, so fall back to extracting a relocation-free
    # .text section with standard binutils tools.
    if command -v "$objcopy_bin" >/dev/null 2>&1 && "$objcopy_bin" -O binary "$object_file" "$binary_file" 2>/dev/null; then
        :
    else
        rm -f "$binary_file"
        if readelf -rW "$object_file" | grep -Eq "Relocation section '.*\\.text'"; then
            echo "$(basename -- "$object_file") has .text relocations; use a cross objcopy or make the test self-contained." >&2
            exit 1
        fi

        read -r text_offset text_size <<EOF
$(readelf -SW "$object_file" | awk '$3 == ".text" { print $6, $7 }')
EOF
        if [ -z "${text_offset:-}" ] || [ -z "${text_size:-}" ]; then
            echo "Could not find a .text section in ${source_file##*/}" >&2
            exit 1
        fi

        dd if="$object_file" of="$binary_file" bs=1 \
            skip=$((16#$text_offset)) count=$((16#$text_size)) status=none
    fi
}

build_c_test() {
    local source_file=$1
    local test_name=$2
    local instruction_set=$3
    local binary_file=$4
    local thumb_payload=$5
    local object_file="$output_dir/$test_name.$instruction_set.o"

    "$clang_bin" --target=arm-none-eabi -march=armv4t "$instruction_set" -O0 \
        -ffreestanding -fno-builtin -fno-stack-protector \
        -fno-unwind-tables -fno-asynchronous-unwind-tables \
        -c "$source_file" -o "$object_file"
    extract_text_section "$object_file" "$binary_file"

    if [ "$thumb_payload" = true ]; then
        payload_file="$output_dir/$test_name.thumb.payload"
        mv "$binary_file" "$payload_file"
        # ARM reset entry: ldr r0, [pc, #0]; bx r0; .word 0x0800000d.
        # The Thumb payload starts at 0x0800000c, and bit 0 selects Thumb.
        printf '\x00\x00\x9f\xe5\x10\xff\x2f\xe1\x0d\x00\x00\x08' > "$binary_file"
        dd if="$payload_file" of="$binary_file" bs=1 seek=12 conv=notrunc status=none
        rm -f "$payload_file"
    fi
    echo "Built ${binary_file#$script_dir/}"
}

for source_file in "${source_files[@]}"; do
    test_name=$(basename -- "${source_file%.*}")

    if [ "${source_file##*.}" = "c" ]; then
        build_c_test "$source_file" "$test_name" -marm "$output_dir/$test_name.bin" false
        build_c_test "$source_file" "$test_name" -mthumb "$output_dir/thumb_$test_name.bin" true
    else
        object_file="$output_dir/$test_name.o"
        binary_file="$output_dir/$test_name.bin"
        "$clang_bin" --target=arm-none-eabi -march=armv4t -marm -c "$source_file" -o "$object_file"
        extract_text_section "$object_file" "$binary_file"
        echo "Built ${binary_file#$script_dir/}"
    fi
done
