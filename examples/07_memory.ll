; Example 7: Memory operations
; This demonstrates stack allocation and memory access.

define i32 @memoryTest() {
entry:
  ; Allocate space for 4 integers on the stack
  %arr = alloca [4 x i32]
  
  ; Get pointers to each element
  %ptr0 = getelementptr [4 x i32], [4 x i32]* %arr, i32 0, i32 0
  %ptr1 = getelementptr [4 x i32], [4 x i32]* %arr, i32 0, i32 1
  %ptr2 = getelementptr [4 x i32], [4 x i32]* %arr, i32 0, i32 2
  %ptr3 = getelementptr [4 x i32], [4 x i32]* %arr, i32 0, i32 3
  
  ; Store values
  store i32 10, i32* %ptr0
  store i32 20, i32* %ptr1
  store i32 30, i32* %ptr2
  store i32 40, i32* %ptr3
  
  ; Load and sum
  %v0 = load i32, i32* %ptr0
  %v1 = load i32, i32* %ptr1
  %v2 = load i32, i32* %ptr2
  %v3 = load i32, i32* %ptr3
  
  %sum1 = add i32 %v0, %v1
  %sum2 = add i32 %v2, %v3
  %total = add i32 %sum1, %sum2
  
  ret i32 %total  ; 10 + 20 + 30 + 40 = 100
}
;;
