use rand::Rng;
use std::cmp::Ordering;
use std::io; //serve per l'output e input

fn main() {
    println!("Guess the number!");
    //there are some change from the rust site:
    let secret_number = rand::rng().random_range(0..=100);
    println!("The secret number is {secret_number}");
    loop {
        println!("Please input your guess.");

        let mut guess = String::new(); //mutable variable

        io::stdin()
            .read_line(&mut guess)
            .expect("Failed to read line");
        //let apple = 40;
        //apple += 50; generate an error cause variable is not mutable
        //println!("{}", apple);

        // trim() delete spaces, parse() instead convert a string to another type
        let guess: u32 = guess.trim().parse().expect("Please type an integer!!");

        println!("You guessed: {guess}");
        match guess.cmp(&secret_number) {
            Ordering::Less => println!("Too small!"),
            Ordering::Greater => println!("Too big!"),
            Ordering::Equal => println!("You win!!!"),
        }
    }
}
