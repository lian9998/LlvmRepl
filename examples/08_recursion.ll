; Example 8: Recursion
; This demonstrates recursive function calls (factorial).

define i32 @factorial(i32 %n) {
entry:
  %isBase = icmp eq i32 %n, 0
  br i1 %isBase, label %base, label %recurse

base:
  ret i32 1

recurse:
  %n.minus.1 = sub i32 %n, 1
  %rec.result = call i32 @factorial(i32 %n.minus.1)
  %result = mul i32 %n, %rec.result
  ret i32 %result
}

define i32 @testFactorial() {
entry:
  %result = call i32 @factorial(i32 5)  ; 5! = 120
  ret i32 %result
}
;;
