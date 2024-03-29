namespace TheNomad::Util {
	class Convert {
		Convert() {
		}
	};


	//
	// M_Random
	// Returns a 0-255 number
	//

	const uint8[] rndtable = {
	    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
	    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
	    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
	    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
	    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
	    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
	    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
	    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
	    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
	    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
	    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
	    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
	    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
	    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
	    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
	    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
	    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
	    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
	    120, 163, 236, 249
	};

	int	rndindex = 0;
	int	prndindex = 0;

	// Which one is deterministic?
	int PRandom() {
	    prndindex = ( prndindex + 1 ) & 0xff;
	    return rndtable[prndindex];
	}

	int MRandom() {
	    rndindex = ( rndindex + 1 ) & 0xff;
	    return rndtable[rndindex];
	}

	void MClearRandom() {
	    rndindex = prndindex = 0;
	}

	int Clamp( int value, int min, int max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}
	
	uint Clamp( uint value, uint min, uint max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}

	float Clamp( float value, float min, float max ) {
		if ( value > max ) {
			return max;
		} else if ( value < min ) {
			return min;
		}
		return value;
	}

	int Hex( char c ) {
		if ( c >= '0' && c <= '9' ) {
			return c - '0';
		} else if ( c >= 'A' && c <= 'F' ) {
			return 10 + c - 'A';
		} else if ( c >= 'a' && c <= 'f' ) {
			return 10 + c - 'a';
		}

		return -1;
	}

	int HexStringToInt( const string& in str )
	{
		if ( str.size() < 1 ) {
			return -1;
		}

		// check for hex code
		if ( str[ 0 ] == '0' && str[ 1 ] == 'x' && str[ 2 ] != '\0' ) {
		    int i, digit, n = 0, len = str.size();

			for ( i = 2; i < len; i++ ) {
				n *= 16;

				digit = Hex( str[ i ] );

				if ( digit < 0 ) {
					return -1;
				}

				n += digit;
			}

			return n;
		}

		return -1;
	}

	bool GetHashColor( const string& in str, vec3& out color )
	{
		uint i, len;
		int[] hex( 6 );

		color[0] = color[1] = color[2] = 0;

		if ( str[1] != '#' ) {
			return false;
		}

		len = str.size();
		if ( len <= 0 || len > 6 ) {
			return false;
		}

		for ( i = 1; i < len; i++ ) {
			hex[i] = Hex( str[i] );
			if ( hex[i] < 0 ) {
				return false;
			}
		}

		switch ( len ) {
		case 3: // #rgb
			color[0] = hex[0] << 4 | hex[0];
			color[1] = hex[1] << 4 | hex[1];
			color[2] = hex[2] << 4 | hex[2];
			break;
		case 6: // #rrggbb
			color[0] = hex[0] << 4 | hex[1];
			color[1] = hex[2] << 4 | hex[3];
			color[2] = hex[4] << 4 | hex[5];
			break;
		default: // unsupported format
			return false;
		};

		return true;
	}

	uint BoolToUInt( bool b ) {
		return b ? 1 : 0;
	}
	
	int BoolToInt( bool b ) {
		return b ? 1 : 0;
	}
	
	bool IntToBool( int64 i ) {
		return i == 1 ? true : false;
	}

	bool UIntToBool( uint64 i ) {
		return i == 1 ? true : false;
	}
	
	bool IntToBool( int32 i ) {
		return i == 1 ? true : false;
	}

	bool UIntToBool( uint32 i ) {
		return i == 1 ? true : false;
	}

	bool IntToBool( int16 i ) {
		return i == 1 ? true : false;
	}

	bool UIntToBool( uint16 i ) {
		return i == 1 ? true : false;
	}

	bool StringToBool( const string& in str ) {
		return StrICmp( str, "true" ) == 0 ? true : false;
	}

	float DEG2RAD( float x ) {
		return ( ( x * M_PI ) / 180.0F );
	}

	float RAD2DEG( float x ) {
		return ( ( x * 180.0f ) / M_PI );
	}

	//
	// Dir2Angle: returns absolute degrees
	//
	float Dir2Angle( TheNomad::GameSystem::DirType dir ) {
		switch ( dir ) {
		case TheNomad::GameSystem::DirType::North:
			return 0.0f;
		case TheNomad::GameSystem::DirType::NorthEast:
			return 45.0f;
		case TheNomad::GameSystem::DirType::East:
			return 90.0f;
		case TheNomad::GameSystem::DirType::SouthEast:
			return 135.0f;
		case TheNomad::GameSystem::DirType::South:
			return 180.0f;
		case TheNomad::GameSystem::DirType::SouthWest:
			return 225.0f;
		case TheNomad::GameSystem::DirType::West:
			return 270.0f;
		case TheNomad::GameSystem::DirType::NorthWest:
			return 315.0f;
		default:
			GameError( "Dir2Angle: invalid dir " + formatUInt( dir ) );
		};
		return -1.0f;
	}

	//
	// Angle2Dir:
	//
	TheNomad::GameSystem::DirType Angle2Dir( float angle ) {
		if ( angle >= 337.5f && angle <= 22.5f ) {
			return TheNomad::GameSystem::DirType::North;
		} else if ( angle >= 22.5f && angle <= 67.5f ) {
			return TheNomad::GameSystem::DirType::NorthEast;
		} else if ( angle >= 67.5f && angle <= 112.5f ) {
			return TheNomad::GameSystem::DirType::East;
		} else if ( angle >= 112.5f && angle <= 157.5f ) {
			return TheNomad::GameSystem::DirType::SouthEast;
		} else if ( angle >= 157.5f && angle <= 202.5f ) {
			return TheNomad::GameSystem::DirType::South;
		} else if ( angle >= 202.5f && angle <= 247.5f ) {
			return TheNomad::GameSystem::DirType::SouthWest;
		} else if ( angle >= 247.5f && angle <= 292.5f ) {
			return TheNomad::GameSystem::DirType::West;
		} else if ( angle >= 292.5f && angle <= 337.5f ) {
			return TheNomad::GameSystem::DirType::NorthWest;
		} else {
			DebugPrint( "Angle2Dir: funny angle " + formatFloat( angle ) + "\n" );
		}
		return TheNomad::GameSystem::DirType::North;
	}
};