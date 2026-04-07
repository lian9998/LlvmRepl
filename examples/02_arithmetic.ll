; Example 2: Arithmetic operations
; This demonstrates basic arithmetic operations in LLVM IR.

define i32 @arithmetic() {
entry:
  %a = add i32 10, 20        ; 10 + 20 = 30
  %b = mul i32 %a, 2         ; 30 * 2 = 60
  %c = sub i32 %b, 10        ; 60 - 10 = 50
  %d = sdiv i32 %c, 5        ; 50 / 5 = 10
  ret i32 %d
}
;;
