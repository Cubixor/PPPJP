# PPPJP dictionary

```
<program> → <statement>* 
<statement> → { 
    kończwaść ( <expr> )
    zmienna <type> `ident` równa <expr> 
}
<type> → { całkowita }
<expr> → {
    <term>
    <bin_expr>
}
<term> → {
    integer_literal
    identifier
}
<arithmetic_expr> → {
    (2) <expr> potęgi <expr> //TODO
    (1) <expr> razy <expr> 
    (1) <expr> podzielić <expr>
    (0) <expr> dodać <expr>
    (0) <expr> odjąć <expr>
}
```

<br>
repetycja → while <br>
jeśliby → if <br>
kończwaść → return

## Code example

```
zmienna całkowita `a1` równa [dwa tysiące dwadzieścia cztery]

jeśliby `a1` było równe [pięć] to:
§1 wyświetlić ( "Tak było równo" )
przeciwnie:
§1 kończwaść [jeden]
```