pub fn str_as_cstr(s: &str) -> alloc::vec::Vec<i8> {
    let mut ret = alloc::vec::Vec::new();
    let mut iter = s.chars();
    while let Some(c) = iter.next() {
        ret.push(c as i8);
    }
    ret.push(0);
    ret
}
