void free_string_vector_memory(std::vector<std::string> &arr) {
    for (auto i : arr)
        i.clear();
    arr.clear();
}