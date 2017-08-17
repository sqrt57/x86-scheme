; SRFI-1: List library
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

(define (fold-left f z lst)
  (if (pair? lst)
      (fold-left f (f z (car lst)) (cdr lst))
      z))

(define (reduce-left f z lst)
  (if (pair? lst)
      (fold-left f (car lst) (cdr lst))
      z))
