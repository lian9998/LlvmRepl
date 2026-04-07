; Example 4: Using printf
; This demonstrates calling external functions like printf.
; The REPL automatically links to the host process's libc.

; Declare printf (external function)
declare i32 @printf(i8*, ...)

; Define a string constant
@.str = private constant [14 x i8] c"Hello, LLVM!\0A\00"

define i32 @hello() {
entry:
  ; Get pointer to the string
  %str = getelementptr [14 x i8], [14 x i8]* @.str, i32 0, i32 0
  
  ; Call printf
  %result = call i32 (i8*, ...) @printf(i8* %str)
  
  ; Return the number of characters printed
  ret i32 %result
}
;;
