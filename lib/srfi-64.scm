; SRFI-64: A Scheme API for test suites
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

(define (test-begin . rest) (write (list "Starting test group" (car rest))))

(define (test-end . rest) (write (list "Finished test group" (car rest))))

(define (test-assert name condition)
  (if condition
      #t
      (write (list name "failed"))))

(define (test-eq name x y)
  (test-assert name (eq? x y)))

(define (test-eqv name x y)
  (test-assert name (eqv? x y)))

(define (test-equal name x y)
  (test-assert name (equal? x y)))
