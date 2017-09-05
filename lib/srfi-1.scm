; SRFI-1: List library
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

(define (fold f z lst)
  (if (pair? lst)
      (fold f (f z (car lst)) (cdr lst))
      z))

(define (reduce f z lst)
  (if (pair? lst)
      (fold f (car lst) (cdr lst))
      z))

(define (fold-right f z lst)
  (if (pair? lst)
      (f (car lst) (fold-right f z (cdr lst)))
      z))

(define (reduce-right f z lst)
  (if (pair? lst)
      (f (car lst) (fold-right f (cdr lst)))
      z))
