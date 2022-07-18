# ash-boot
A compiler for a custom programming language.

#### Building The Compiler
If you don't currently have the repo run `git clone --recurse-submodules https://github.com/cameronmcnz/surface`
to clone this repo and the llvm repo.

Then to build llvm follow one of the commands for either Windows or Linux in
[llvm_build_commands.txt](./llvm_build_commands.txt).

To build the compiler you need to run `cmake` in the `stage-0-compiler` folder.

#### Running The Compiler
The syntax for running the compiler is:
- `./ash-boot-stage0 <input-file> <output-file>` for Linux
- `ash-boot-stage0.exe <input-file> <output-file>` for Windows

The input file can be any file that contains the source of the program to build.
The output file will by default contain the ir code, but can be changed using the `--output-type` option.

###### Options
- `--output-type=[type]` chooses what type the output file will be, supported values are `ir` or `obj`.

##### Building The Result
To build from the IR code, first run llc from either the system path, or the one created when building llvm.

`llc -filetype=obj <input-file>`

Now the object file which was built from the above command, or from the program itself, can be compiled into a executable file.

For Linux use either `clang` or `gcc` to build the object file.

`gcc -o <output-executable> <input-file>`

Or for Windows first make sure you have either opened the Visual Studio command prompt, or run `VsDevCmd.bat`.
Then the code can be compiled using `link.exe`

`link.exe <input-file> /defaultlib:libcmt /OUT:<output-executable>`

#### Sample Program
```
# module declaration
module sample;

# externally defined functions
extern int putchar(int c);

function int add(int x, int y) {
	x + y;
}

# functions
function int main() {
	# variables
	var int i = 10;
	var float f = 1.5f;
	var bool b = true;

	# nested function (temp disabled)
	#function int add(int x, int y) {
	#	x + y;
	#}

	# function call
	var int r = add(i, 7);

	# if statement
	if (r > 15) {
		r = r * 1;
	} else {
		r = r / 2;
	}

	# inline if
	var int bv = if b {1;} else {2;};
	
	# for loop
	for int i = 0; i< 10; i = i+1 {
		
	}
	
	# while loop
	while i < 100 {
		i = i + 1;
	}
	
	# scope operator
	sample::putchar(97);
	
	# unary operator
	var int z = -i;
	
	# return value
	0;
}
```