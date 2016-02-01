<?php
  class MyClass{
     /*int*/ $a;
     /*double*/ $b;

    function /*int*/ getA(){
      return $this->a;
    }

    function /*double*/ getB(){
      return $this->b;
    }

    function /*void*/ setA(/*int*/ $v){
      $this->a=$v;
    }

    function /*void*/ setB(/*double*/ $v){
      $this->b=$v;
    }
  }
?>

<?php
function /*void*/ my_function(){
  /*int*/ $v=4;
  echo $v;
  echo $x;
}
?>

<?php
/*int*/ $x=3;
$x+=4+4;
echo $x;
my_function();
/*c++:MyClass*/ foo /*php:*/= new MyClass/**/;
foo->setA(4);
echo foo->getA();
?>
