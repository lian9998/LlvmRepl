; Example 6: Loops
; This demonstrates a simple loop that computes a sum.

define i32 @sumToN(i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %sum = phi i32 [ 0, %entry ], [ %sum.next, %loop ]
  
  %i.next = add i32 %i, 1
  %sum.next = add i32 %sum, %i.next
  
  %done = icmp eq i32 %i.next, %n
  br i1 %done, label %exit, label %loop

exit:
  ret i32 %sum.next
}

define i32 @testSum() {
entry:
  %result = call i32 @sumToN(i32 10)  ; 1+2+...+10 = 55
  ret i32 %result
}
;;
