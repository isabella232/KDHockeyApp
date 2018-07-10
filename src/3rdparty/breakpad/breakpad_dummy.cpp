// XCode doesn't accept empty static libraries without object files, or symbols.
// So let's define a dummy function to make it happy.
void breakpad_dummy() {}
