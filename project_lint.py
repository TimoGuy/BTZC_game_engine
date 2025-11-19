from pathlib import Path


def find_src_entries() -> list[str]:
    CMAKELISTS_FNAME = './CMakeLists.txt'
    START_BLOCK = 'set(MAIN_SOURCES'
    END_BLOCK = ')'
    SRC_FILE_ENTRY_START = '${CMAKE_CURRENT_SOURCE_DIR}'

    block_process = 0  # 0:before; 1:within; 2:after;
    found_source_files = []
    with open(CMAKELISTS_FNAME) as f:
        for line in f:
            if block_process == 0:
                if line.strip() == START_BLOCK:
                    # Found start of block.
                    block_process = 1
            elif block_process == 1:
                if line.strip().startswith(SRC_FILE_ENTRY_START):
                    # Add file entry.
                    entry_start_str_len = len(SRC_FILE_ENTRY_START)+1  # +1 for '/' after.
                    found_source_files.append(line.strip()[entry_start_str_len:])
                elif line.strip() == END_BLOCK:
                    # Found end of block.
                    block_process = 2
            elif block_process == 2:
                # Exit reading file.
                break

    return found_source_files


def find_existing_files() -> list[Path]:
    SEARCH_DIRS = ['./src/']
    SEARCH_EXTENSIONS = ['h',
                         'hpp',
                         'ixx',  # I think this is for modules???
                         'c',
                         'cxx',
                         'cpp']
    all_found_files = []
    for search_dir in SEARCH_DIRS:
        for search_ext in SEARCH_EXTENSIONS:
            # Convert `search_ext` to case-insensitive extension.
            case_insensitive_ext = '*.'
            for ext_char in search_ext:
                case_insensitive_ext += f'[{ext_char.lower()}{ext_char.upper()}]'

            # Search directory for extension.
            files = list(Path(search_dir).rglob(case_insensitive_ext))
            all_found_files.extend(files)

    return all_found_files


def find_missing_src_entry_in_src_entries(src_entries: list[str],
                                          existing_files: list[Path]):
    missing_entries = []  # If the file exists but not the entry in cmake, then append!
    src_entry_paths = [Path(x) for x in src_entries]
    for existing_file in existing_files:
        if existing_file not in src_entry_paths:
            # Found missing entry.
            missing_entry = str(existing_file).replace('\\', '/')
            missing_entries.append(missing_entry)

    missing_entries.sort()
    return missing_entries


if __name__ == '__main__':
    src_entries = find_src_entries()
    existing_files = find_existing_files()

    files_missing_in_src_entries = \
        find_missing_src_entry_in_src_entries(src_entries, existing_files)

    # Print result.
    if len(files_missing_in_src_entries) > 0:
        print('==== MISSING ENTRIES ============================================')
    for missing_file in files_missing_in_src_entries:
        print(missing_file)
