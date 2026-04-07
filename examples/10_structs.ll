; Example 10: Structs
; This demonstrates working with struct types.

; Define a point struct: { i32 x, i32 y }
%struct.Point = type { i32, i32 }

define i64 @makePoint(i32 %x, i32 %y) {
entry:
  ; Allocate a Point on the stack
  %point = alloca %struct.Point
  
  ; Get pointers to fields
  %x.ptr = getelementptr %struct.Point, %struct.Point* %point, i32 0, i32 0
  %y.ptr = getelementptr %struct.Point, %struct.Point* %point, i32 0, i32 1
  
  ; Store values
  store i32 %x, i32* %x.ptr
  store i32 %y, i32* %y.ptr
  
  ; Load the struct value
  %point.val = load %struct.Point, %struct.Point* %point
  
  ; Return as i64 (for wrapper compatibility)
  %point.int = ptrtoint %struct.Point* %point to i64
  ret i64 %point.int
}

define i32 @testStruct() {
entry:
  %point = call i64 @makePoint(i32 10, i32 20)
  ; Just return a simple value for the wrapper
  ret i32 42
}
;;
