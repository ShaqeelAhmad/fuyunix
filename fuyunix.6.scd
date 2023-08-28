fuyunix(6)

# NAME

_fuyunix_ [*-v*|*-l*|*-f*]

# DESCRIPTION
	fuyunix is a simple platformer game. It has local multiplayer support
	(currently only focused on 2 players support).

# OPTIONS
*-v*
	Shows the version.

*-f*
	Start game in fullscreen.

*-l*
	List all functions for keys configuration in config file.

# ENVIRONMENT VARIABLES
*XDG_STATE_HOME*
	Is used for saving game state.

*XDG_CONFIG_HOME*
	Is used for configuration files.

*HOME*
	If the above environment variables are not set, $HOME/.fuyunix will be used
	for those purposes instead.

# FILES
_$XDG_STATE_HOME/fuyunix/save_
	Game state saved in binary file.

_$XDG_CONFIG_HOME/fuyunix/config_
	The configuration file.

# DEFAULT KEYS
TODO

# CONFIGURATION
TODO
