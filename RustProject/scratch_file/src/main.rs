use std::io;
// fn main() {
//     // // const PI: i32 = 3;
//     //
//     // let x = 4;
//     //
//     // {
//     //     let x = x * 2;
//     //     println!("Value of x in a inner scope: {x}");
//     // }
//     // println!("Value of x in a inner scope: {x}");
//     //
//     // // let mut spaces = "    ";
//     // //  spaces  = space.len();
//     //
//     // //  generate an error cause the variable is mutuable and
//     // //  cannot allocate to the variable a different type
//     //
//     // let spaces = "   ";
//     // let _spaces = spaces.len(); //with underscore we can fix the waring of unused variable
//     // //this alternative work cause you allocate to the memory directly the new type
//     //
//     // //COMPOUND TYPES
//     //
//     // //tuple
//     // let tup = (500, 30, 0.0002);
//     // let (x, y, z) = tup;
//     // println!("{x}");
//     // println!("{y}");
//     // println!("{z}");
//     // //you can acces to a single element of a tuple like this
//     // println!("first number: {}", tup.0);
//
//     println!("Insert the value");
//     let mut input_string = String::new();
//
//     io::stdin()
//         .read_line(&mut input_string)
//         .expect("Failed to read the value");
//     let value: i64 = input_string
//         .trim()
//         .parse()
//         .expect("Input is not an integer");
//     println!("this is the square of your value: {}", square(value));
//
//     is_odd(value);
// }
//
// fn square(x: i64) -> i64 {
//     x * x
// }
//
// fn is_odd(x: i64) -> i64 {
//     if x % 2 == 0 {
//         println!("The number you input is NOT odd!");
//         0 // Return 0 for even
//     } else {
//         println!("The number you input is odd!");
//         1 // Return 1 for odd
//     }
// }

fn main() {
    let a = [10, 20, 30, 40, 50];
    for el in a {
        println!("Element printed: {el}\n");
    }
    //.rev() reverse the order
    for number in (1..10) {
        println!("{number}");
    }
}
