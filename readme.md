# PPPJP programming language
### Polish Truly Patriotic Progamming Language (PL: <u>P</u>olski <u>P</u>rawdziwie <u>P</u>atriotyczny <u>J</u>ęzyk <u>P</u>rogramowania)


## About
PPPJP has been developed to fill a significant gap in the programming language market. There are dozens of popular and versatile programming langages, but they all have a substantial drawback - their syntax is written in English. How can a Polish citizen be a programmer in such a situation, while maintaining a patriotic attitude, love for the native language, culture and national heritage? The solution to this problem is PPPJP - a modern, high level programming language combining the key advantages of all other popular programming languages and the beauty of the best of natural languages - Polish. Its syntax, documentation and compiler logs are all in Polish so that the English language knowledge is no longer required for a Pole to become a programmer. 🫡

## Technical Details
Only `Linux` operating system with a `x86_64` cpu is currently supported.

The compiler requires `nasm` and `ld` to compile the file.

To compile a file run a `pppjp <file.pppp>` command

## Code example
The code here doesn't make sense, (although it will compile and run), it's only to demonstrate the syntax. Some actually useful pieces of code can be found in the `examples` folder.
#### PPPJP Code
```
# To jest komentarz

zmienna całkowita `a` równa [dwa tysiące sto trzydzieści siedem]
zmienna logiczna `b` równa prawda
zmienna znak `c` równa 'A'
tablica znak `d` równa {'L', 'O', 'L'}
tablica całkowita `e` rozmiaru [pięć]

`e` element [zero] równa [jeden]
`e` element [cztery] równa ((`a` modulo [cztery]) razy [osiem] dodać [sześć])  odjąć [dwanaście] podzielić [minus sześć]

jeśli (`a` większe [sto] oraz `b`): {
	wyświetl_liczbę(`e` element [cztery])
} przeciwnie jeśli ( nie (prawda lub fałsz) równe ((nie prawda) oraz (nie fałsz)) ): {
	wyświetl_znak('!')
} przeciwnie: {
	kończwaść([jeden])
}

powtarzaj:
	przerwij

powtarzaj jeśli (`a` mniejszerówne [dwa tysiące sto czterdzieści]): {
    `a` równa `a` dodać [jeden]
	jeśli (`a` różne [dwa tysiące sto trzydzieści osiem]): {
		kontynuuj
	}
}
```

#### C++ Code (for comparison)
```
// This is a comment

int a = 2137;
bool b = true;
char c = 'A';
char d[] = {'L', 'O', 'L'};
int e[5];

e[0] = 1;
e[4] = ((a % 4) * 8 + 6) - 12 / -6; 

if(a>100 && b){
	std::cout<<e[4];
}else if (!(true || false) == (!(true) && !(false))){
	std::cout<<'!';
}else{
	exit(1);
}

while(true)
	break;

while(a<=2040){
    a++;
	if(a!=2138){
		continue;
	}
}
```


## Disclaimer
The foundation of the compiler is based on the amazing tutorial from [Pixeled](https://www.youtube.com/playlist?list=PLUDlas_Zy_qC7c5tCgTMYq2idyyT241qs) and his project [Hydrogen](https://github.com/orosmatthew/hydrogen-cpp). That's where I gained the basic knowledge about how compilers and assembly language work. Despite this, I am still a beginner in this field as well as in the c++ language, so do not take this project as a model example but rather as a joke. It still has many shortcomings and has not been thoroughly tested so there may be some bugs that came unnoticed.