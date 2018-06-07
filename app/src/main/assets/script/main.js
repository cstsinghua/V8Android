log("hello javascript");

log("global obj globalGamer age property: "+ globalGamer.age)
log("global obj globalGamer name property: "+ globalGamer.name)

log("global obj globalGamer getAge: "+ globalGamer.getAge())
log("global obj globalGamer getName: "+ globalGamer.getName())

//C++ invoke JS function and pass in C++ instance obj
function Game(gamer){
    log("gamer's age by accessing field is: "+ gamer.age);
    log("gamer's name by accessing field is: "+ gamer.name);

    log("gamer's age by accessing instance function is: "+ gamer.getAge());
    log("gamer's name by accessing instance function is: "+ gamer.getName());

}
