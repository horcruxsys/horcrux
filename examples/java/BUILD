# examples/java/BUILD
# Horcrux BUILD file for the Java example project (non-Android).

java_library(
    name = "greeter",
    srcs = ["src/com/example/Greeter.java"],
)

java_binary(
    name = "hello",
    srcs = ["src/com/example/Main.java"],
    main_class = "com.example.Main",
    deps = [":greeter"],
)

java_test(
    name = "greeter_test",
    srcs = ["src/com/example/GreeterTest.java"],
    main_class = "com.example.GreeterTest",
    deps = [":greeter"],
)
