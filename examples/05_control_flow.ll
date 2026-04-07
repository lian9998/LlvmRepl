; Example 5: Control flow
; This demonstrates branching and comparisons in LLVM IR.

define i32 @abs(i32 %x) {
entry:
  %isNegative = icmp slt i32 %x, 0
  br i1 %isNegative, label %negate, label %done

negate:
  %negated = sub i32 0, %x
  br label %done

done:
  %result = phi i32 [ %x, %entry ], [ %negated, %negate ]
  ret i32 %result
}

define i32 @testAbs() {
entry:
  %1 = call i32 @abs(i32 -42)
  ret i32 %1
}
;;
