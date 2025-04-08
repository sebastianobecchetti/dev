use std::io; //serve per l'output e input


fn main() {
    println!("Guess the number!");

    println!("Please input your guess.");

    let mut guess = String::new(); //mutable variable
    //let apple = 40;

    io::stdin()
        .read_line(&mut guess)
        .expect("Failed to read line");
    //apple += 50; generate an error cause variable is not mutable
    println!("You guessed: {}", guess);
    //println!("{}", apple);
}

