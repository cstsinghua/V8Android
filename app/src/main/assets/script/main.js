log("hello javascript");

function Game(gamer){
    log("gamer's age by accessing field is: "+ gamer.age);
    log("gamer's name by accessing field is: "+ gamer.name);

    log("gamer's age by accessing instance function is: "+ gamer.getAge());
    log("gamer's name by accessing instance function is: "+ gamer.getName());

}