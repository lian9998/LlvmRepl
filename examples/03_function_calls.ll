; Example 3: Function calls
; This demonstrates calling one function from another.
; Note: Functions are remembered across submissions!

; First, define a helper function
define i32 @helper() {
entry:
  ret i32 100
}
;;

; Then define a function that calls it
define i32 @caller() {
entry:
  %1 = call i32 @helper()
  %2 = add i32 %1, 50
  ret i32 %2
}
;;
