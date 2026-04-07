; Example 9: Floating point operations
; This demonstrates floating point arithmetic.

define double @circleArea(double %radius) {
entry:
  ; Area = π * r²
  %r.squared = fmul double %radius, %radius
  %pi = fptrunc double 0x400921FB54442D18 to double  ; π
  %area = fmul double %pi, %r.squared
  ret double %area
}

define double @testArea() {
entry:
  %area = call double @circleArea(double 2.0)  ; π * 4 ≈ 12.57
  ret double %area
}
;;
