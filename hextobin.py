from intelhex import IntelHex


def hex_to_bin(hex_file, bin_file):
    ih = IntelHex(hex_file_path)

    ih.tobinfile(bin_file_path)

if __name__ == '__main__':
    hex_file_path = 'path/to/your/hex/file.hex'
    bin_file_path = 'path/to/your/bin/file.bin'

    hex_to_bin(hex_file_path, bin_file_path)
    print(f"Converted {hex_file_path} to {bin_file_path}")