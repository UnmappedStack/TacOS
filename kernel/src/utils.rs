pub fn str_as_cstr(s: &str) -> alloc::vec::Vec<i8> {
    let mut ret = alloc::vec::Vec::new();
    let mut iter = s.chars();
    while let Some(c) = iter.next() {
        ret.push(c as i8);
    }
    ret.push(0);
    ret
}

pub fn cstr_as_string(s: alloc::vec::Vec<i8>) -> alloc::string::String {
    let mut ret = alloc::string::String::new();
    for i in s.iter() {
        ret.push(*i as u8 as char); // lol
    }
    ret
}
