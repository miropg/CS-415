#!/bin/bash

# NOTE: Most tests rely on an already working interactive mode and
# a working Makfile
#
#

EXECUTABLE="./pseudo-shell"

TEST_DIR="test_pseudo_shell"
FILE1="test_file1.txt"
FILE2="test_file2.txt"

# Setup test environment
setup_test_environment() {
    mkdir $TEST_DIR
    cp $EXECUTABLE $TEST_DIR
    echo "This is a test file." > $TEST_DIR/$FILE1
    echo "This is another test file." > $TEST_DIR/$FILE2
}

cleanup_test_environment() {
    rm -drf $TEST_DIR
}


test_ls_command() {
    echo "Testing 'ls' command..."
    cd $TEST_DIR

    
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
ls
exit
EOF
    )
    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
ls
exit
EOF
    )

    # Define the expected output
    expected_output=">>> .. . test_file1.txt pseudo-shell test_file2.txt 
>>> "

    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'ls'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'ls'."
    else
        :
    fi

    outSorted=

    # Compare the expected output with the pseudo-shell output
    if [ $(echo "$pseudo_shell_output" | grep -o . | sort | tr -d  "\n") == $(echo "$expected_output" | grep -o . | sort | tr -d  "\n" ) ]; then
        echo "'ls' command output matches expected output."
    else
        echo "ERROR: 'ls' command output does not match expected output. it does not have to be in order but this is expected output"

        echo "$expected_output"

        echo "Your output"
        echo "$pseudo_shell_output" > out.txt

        echo "Note: make sure you are listing all files with space inbetween. Also print the prompt like this '>>> '"
    fi

    echo ""

    cd ..
}


# Function to test 'cat' command
test_cat_command() {
    echo "Testing 'cat' command..."
    cd $TEST_DIR

    # Prepare the test file and content
    echo "Sample content for cat test." > test_cat_file.txt

    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
cat test_cat_file.txt
exit
EOF
    )
    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
cat test_cat_file.txt
exit
EOF
    )

    expected_output=">>> Sample content for cat test.
>>> "

    # Capture any valgrind memory errors or leaks
    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'cat'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'cats'."
    else
        :
    fi

    # Compare the expected output with the pseudo-shell output
    if [ "$pseudo_shell_output" == "$expected_output" ]; then
        echo "'cat' command output matches expected output."
        :
    else
        echo "Error: 'cat' command output does not match expected output."
        diff -u <(echo "$pseudo_shell_output") <(echo "$expected_output")
        :
    fi

    echo ""
    cd ..
}

test_pwd_command() {
    echo "Testing 'pwd' command..."
    cd $TEST_DIR

    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
pwd
exit
EOF
    )
    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
pwd
exit
EOF
    )


    # Define the expected output, assuming $TEST_DIR is an absolute path
    expected_output=">>> $PWD
>>> "

    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'pwd'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'pwd'."
    else
        :
    fi

    # Compare the expected output with the pseudo-shell output
    if [ "$pseudo_shell_output" == "$expected_output" ]; then
        echo "'pwd' command output matches expected output."
        :
    else
        echo "Error: 'pwd' command output does not match expected output."
        diff -u <(echo "$pseudo_shell_output") <(echo "$expected_output")
        :
    fi

    echo ""

    cd ..
}

test_mkdir_command() {
    echo "Testing 'mkdir' command..."
    cd $TEST_DIR

    # Define the name of the new directory to create
    NEW_DIR="test_new_dir"

    # Run the 'mkdir' command via the pseudo-shell executable and check for memory issues using EOF format
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
mkdir $NEW_DIR
exit
EOF
    )
    rm -drf $TEST_DIR
    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
mkdir $NEW_DIR
exit
EOF
    )

   

    # Check if the new directory was successfully created
    dir_created="[ -d "$NEW_DIR" ] && echo 'Directory created' || echo 'Directory not created'"

    # Check if there were memory errors or leaks reported by valgrind
    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'mkdir'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'mkdir'."
    else
        :
    fi

    # Compare the expected directory creation with the actual result and report memory leaks
    if [ -d "$NEW_DIR" ]; then
        echo "'mkdir' command output matches expected output."
    else
        echo "Error: 'mkdir' command failed to create $NEW_DIR directory."
        #echo "$mem_leak_msg"
    fi

    if [ ! -d "$NEW_DIR" ]; then
        echo "Expected output: 'Directory created'"
        echo "Actual output from pseudo-shell: $(eval $dir_created)"
        diff -u <(echo "Directory created") <(echo "$(eval $dir_created)")
    fi

    echo ""


    cd ..
}

test_cd_command() {
    echo "Testing 'cd' command..."
    cd $TEST_DIR

    # Create a directory to change into
    NEW_DIR="test_cd_dir"
    mkdir $NEW_DIR

    # Define the expected output, assuming $TEST_DIR/NEW_DIR is the absolute path to NEW_DIR
    expected_output=">>> $PWD/$NEW_DIR
>>> "

    # Run the 'cd' command followed by 'pwd' via the pseudo-shell executable and check for memory issues
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
cd $NEW_DIR
pwd
exit
EOF
    )

    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
cd $NEW_DIR
pwd
exit
EOF
    )

    # Capture any valgrind memory errors or leaks
     mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'cd'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'cd'."
    else
        :
    fi

    # Compare the expected output with the pseudo-shell output
    if [[ "$pseudo_shell_output" == *"$expected_output"* ]]; then
        echo "'cd' command output matches expected output."
    else
        echo "Error: 'cd' command followed by 'pwd' output does not match expected output."
        diff -u <(echo "$pseudo_shell_output") <(echo "$expected_output")
        echo "THIS TEST RELYS ON A WORKING PWD"
    fi

    echo ""

    cd ..
}

test_rm_command() {
    echo "Testing 'rm' command..."
    cd $TEST_DIR

    # Create a file to remove
    FILE_TO_REMOVE="test_rm_file.txt"
    echo "This file will be removed." > "$FILE_TO_REMOVE"

    # Run the 'rm' command via the pseudo-shell executable and check for memory issues
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
rm $FILE_TO_REMOVE
exit
EOF
    )

    # Capture any valgrind memory errors or leaks
    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')

    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'rm'."
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'rm'."
    else
        :
    fi
    

    # Verify the file was successfully removed
    if [ ! -f "$FILE_TO_REMOVE" ]; then
        echo "'rm' command successfully removed the file."
    else
        echo "Error: 'rm' command failed to remove the file."
    fi
    
    echo ""

    cd ..
}


#---------------------------
# CP tests

test_cp_base() {

    local description="$1"
    local src="$2"
    local dst="$3"
    local expected_dst="$4"

    echo "=== Testing 'cp' command: $description ==="

    if [[ -d "$src" ]]; then
        mkdir -p "$src"
    else
        echo "Content of the source file." > "$src"
    fi

    echo "Executing: cp $src $dst"
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
cp $src $dst
exit
EOF
    )
    

    process_valgrind_output "$valgrind_output"
    verify_copy "$src" "$expected_dst" "$description"

    echo ""
    cd ..
}

process_valgrind_output() {
    local valgrind_output="$1"
    local mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    local leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')

    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "  - Memory errors detected!"
    fi

    if [ -n "$leaks_detected" ]; then
        echo "  - Memory leaks detected!"
    else
        :
    fi
}

verify_copy() {
    local src="$1"
    local dst="$2"
    local description="$3"
    if [ -f "$dst" ] || [ -d "$dst" ]; then
        if ! diff "$src" "$dst" > /dev/null; then
            echo "  - ERROR: Contents do not match for scenario '$description'."
        else
            echo "'cp' command successfully copied for scenario '$description'."
        fi
    else
        echo "ERROR: 'cp' command failed to copy for scenario '$description'."
    fi
}


test_cp_file_to_file() {
    cd $TEST_DIR
    test_cp_base "file to another file" "source_file.txt" "copied_file.txt" "copied_file.txt"
}

test_cp_file_to_dir() {
    cd $TEST_DIR
    mkdir -p "target_dir"
    test_cp_base "file to directory" "source_file.txt" "target_dir/" "target_dir/source_file.txt"
}

test_cp_file_to_dir_file() {
    cd $TEST_DIR
    mkdir -p "target_dir"
    echo "Content for file to dir/file test." > "source_file3.txt"
    test_cp_base "file to dir/file" "source_file3.txt" "target_dir/source_file3.txt" "target_dir/source_file3.txt"
}

test_cp_dir_file_to_current() {
    cd $TEST_DIR
    mkdir -p "source_dir"
    echo "Content for dir/file test." > "source_dir/current.txt"
    test_cp_base "dir/file to current directory" "source_dir/current.txt" "../$TEST_DIR/" "current.txt"
}

test_cp_dir_file_to_dir_file() {
    cd $TEST_DIR
    echo "Content for dir/file test." > "source_dir/source_file2.txt"
    test_cp_base "dir/file to dir/file" "source_dir/source_file2.txt" "target_dir/file2.txt" "target_dir/file2.txt"
}

test_cp_file_to_subdir() {
    cd $TEST_DIR
    mkdir -p "subdir"
    test_cp_base "file to subdir" "source_file.txt" "subdir/" "subdir/source_file.txt"
}

#---------------------------
# MV
test_mv_base() {
    local description="$1"
    local src="$2"
    local dst="$3"
    local expected_dst="$4"

    echo "=== Testing 'mv' command: $description ==="

    # Setup source
    if [[ -d "$src" ]]; then
        mkdir -p "$src"
    else
        echo "Content of the source file." > "$src"
    fi

    echo "Executing: mv $src $dst"
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
mv $src $dst
exit
EOF
    )

    process_valgrind_output "$valgrind_output"
    verify_move "$src" "$expected_dst" "$description"

    echo ""
    cd ..
}

verify_move() {
    local src="$1"
    local dst="$2"
    local description="$3"
    if [ -f "$dst" ] || [ -d "$dst" ]; then
        if [ -e "$src" ]; then
            echo "ERROR: 'mv' command failed to remove the source for scenario '$description'."
        else
            echo "'mv' command successfully moved for scenario '$description'."
        fi
    else
        echo "ERROR: 'mv' command failed to move for scenario '$description'."
    fi
}

test_mv_file_to_file() {
    cd $TEST_DIR
    echo "HEY how are you" > "source_file.txt"
    test_mv_base "file to another file" "source_file.txt" "moved_file.txt" "moved_file.txt"
}

test_mv_file_to_dir() {
    cd $TEST_DIR
    mkdir -p "target_dir"
    echo "HEY how are you" > "source_file.txt"
    test_mv_base "file to directory" "source_file.txt" "target_dir/" "target_dir/source_file.txt"
}

test_mv_file_to_dir_file() {
    cd $TEST_DIR
    mkdir -p "target_dir"
    echo "Content for file to dir/file test." > "source_file3.txt"
    test_mv_base "file to dir/file" "source_file3.txt" "target_dir/moved_source_file3.txt" "target_dir/moved_source_file3.txt"
}

test_mv_dir_file_to_current() {
    cd $TEST_DIR
    mkdir -p "source_dir"
    echo "Content for dir/file test." > "source_dir/current.txt"
    test_mv_base "dir/file to current directory" "source_dir/current.txt" "./moved_current.txt" "moved_current.txt"
}

test_mv_dir_file_to_dir_file() {
    cd $TEST_DIR
    mkdir -p "source_dir" "target_dir"
    echo "Content for dir/file test." > "source_dir/source_file2.txt"
    test_mv_base "dir/file to dir/file" "source_dir/source_file2.txt" "target_dir/moved_file2.txt" "target_dir/moved_file2.txt"
}

test_mv_file_to_subdir() {
    cd $TEST_DIR
    mkdir -p "subdir"
    echo "tee hee" > "hehe.txt"
    test_mv_base "file to subdir" "hehe.txt" "subdir/" "subdir/hehe.txt"
}

#---------------------------

#Test multiple commands

test_multiple_commands() {
    cd $TEST_DIR

    # Run the 'ls' command via the pseudo-shell executable and check for memory issues using EOF format
    valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
ls; ls; ; ls ; 
exit
EOF
    )
    pseudo_shell_output=$(../$EXECUTABLE <<-EOF
ls; ls; ; ls ; 
exit
EOF
    )

    # Define the expected output
    expected_output=">>> .. . pseudo-shell test_file1.txt test_file2.txt 
.. pseudo-shell . test_file1.txt test_file2.txt 
.. pseudo-shell . test_file1.txt test_file2.txt 
>>> "


    mem_errors=$(echo "$valgrind_output" | grep 'ERROR SUMMARY:')
    leaks_detected=$(echo "$valgrind_output" | grep -Eo 'definitely lost: [^0]|indirectly lost: [^0]|possibly lost: [^0]|still reachable: [^0]')
    # Check if there were memory errors or leaks reported by valgrind

    echo "=== Testing Multiple Commands in the same line ==="
    if echo "$mem_errors" | grep -q "0 errors"; then
        :
    else
        echo "Memory errors detected in 'multiple commands same line'"
    fi

    if [ -n "$leaks_detected" ]; then
        echo "Memory leaks detected in 'multiple commands same line'"
    else
        :
    fi

    # Compare the expected output with the pseudo-shell output
    if [ $(echo "$pseudo_shell_output" | grep -o . | sort | tr -d  "\n") == $(echo "$expected_output" | grep -o .| sort | tr -d  "\n" ) ]; then
        echo "'multiple commands on the same line' command output matches expected output."
    else
        echo "Error: 'multiple commands same line' command output does not match expected output."

        diff -u <(echo "$pseudo_shell_output") <(echo "$expected_output")
    fi

    echo ""

    cd ..
}


test_error_handling() {
    cd $TEST_DIR

    echo "=== Testing Error Handling ==="

    echo "Testing with invalid command..."
    test_invalid_command

    echo "Testing with too many arguments..."
    test_too_many_arguments

    cd ..
}

test_invalid_command() {
    local valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
lsl
exit
EOF
    )
    local pseudo_shell_output=$(../$EXECUTABLE 2>&1 <<-EOF
lsl
exit
EOF
    )

    echo "Checking for 'Error' in output for invalid command..."
    if echo "$pseudo_shell_output" | grep -q "Error"; then
        echo "Success: 'Error' detected for invalid command."
    else
        echo "ERROR: 'Error' not detected when expected."
    fi

    process_valgrind_output "$valgrind_output"

    echo ""
}

test_too_many_arguments() {
    local valgrind_output=$(valgrind ../$EXECUTABLE 2>&1 <<-EOF
ls arg1 arg2
exit
EOF
    )
    local pseudo_shell_output=$(../$EXECUTABLE 2>&1 <<-EOF
ls arg1 arg2
exit
EOF
    )

    echo "Checking for 'Error' in output for too many arguments..."
    if echo "$pseudo_shell_output" | grep -q "Error"; then
        echo "Success: 'Error' detected for command with too many arguments."
    else
        echo "ERROR: 'Error' not detected when expected for too many arguments."
    fi

    process_valgrind_output "$valgrind_output"

    echo ""
}

test_file_mode() {
    echo "=== Testing File Mode ==="
    cd $TEST_DIR

    # Prepare an input file with commands
    echo "ls
ls
ls" > input.txt

    echo ".. expected_output.txt pseudo-shell input.txt . output.txt test_file1.txt test_file2.txt 
.. expected_output.txt input.txt pseudo-shell . output.txt test_file1.txt test_file2.txt 
.. expected_output.txt input.txt pseudo-shell . output.txt test_file1.txt test_file2.txt " > expected_output.txt

    # Execute the pseudo-shell in file mode with the input file and check for memory issues using Valgrind
    valgrind_output=$(valgrind ../$EXECUTABLE -f input.txt 2>&1)

    # Analyze Valgrind output for memory errors or leaks
    process_valgrind_output "$valgrind_output"

    # Now compare out.txt with the expected output
    if cmp -s <(grep -o . output.txt | sort) <(grep -o . expected_output.txt | sort); then
        echo "Success: Output matches expected output."
    else
        echo "ERROR: output.txt does not match expected output."
        echo "DEV NOTE: there is no '>>>' in file mode"
        diff -u output.txt expected_output.txt
    fi

    echo ""
    cd ..
}


#---------------------------

# Compile the program
make clean
rm psuedo-shell
rm -drf $TEST_DIR
make
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Compilation failed, executable not found."
    exit 1
fi

# Start the tests
echo ""
echo "Dev Notes: Make sure you are printing '>>> ' when prompting.
ls has spaces inbetween each item it lists and one at the end.
Dont forget new line characters.
Tests are dependent if exit is implemented"

setup_test_environment

echo ""

test_ls_command
test_cat_command
test_pwd_command
test_mkdir_command
test_cd_command
test_rm_command

echo "----------------------------------"
test_cp_file_to_file
test_cp_file_to_dir
test_cp_file_to_dir_file
test_cp_dir_file_to_current
test_cp_dir_file_to_dir_file
test_cp_file_to_subdir
echo "----------------------------------"

cleanup_test_environment
setup_test_environment

echo "----------------------------------"
test_mv_file_to_file
test_mv_file_to_dir
test_mv_file_to_dir_file
test_mv_dir_file_to_current
test_mv_dir_file_to_dir_file
test_mv_file_to_subdir
echo "----------------------------------"

cleanup_test_environment
setup_test_environment
echo ""

test_multiple_commands

test_error_handling

cleanup_test_environment
setup_test_environment
test_file_mode

cleanup_test_environment
echo "All tests completed."
