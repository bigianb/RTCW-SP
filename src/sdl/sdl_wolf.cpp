
// GOG installer makes it upper case
const char* BASEGAME = "Main";
const char* gameConfigName = "wolfconfig.cfg";

extern int common_main( int argc, char **argv );

int main( int argc, char **argv )
{
    return common_main( argc, argv );
}
