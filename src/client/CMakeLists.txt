set(CLIENT_SOURCES
    main.cpp
)

find_package(Threads REQUIRED)

add_executable(ChatClient ${CLIENT_SOURCES})

target_link_libraries(ChatClient PRIVATE
    Threads::Threads
)
