
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <err.h>
#include <math.h>

#include <openssl/crypto.h>
#include <openssl/whrlpool.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include "sha3.c"

//from streebog.net
#include "gost3411-2012-core.h"

#define STDIN_BUFFER_SIZE 2048
#define PRIMITIVE_OUTPUT_BYTES 64
#define CHYMOS_OUTPUT_BYTES 256

// For speed, we have precomputed the primes [P4495, P6541] (the largest 2,047 primes below 2^16)
// the primes are pseudo-orthogonal and serve as "nothing up the sleeve" numbers
uint16_t thetas[2047] = {
	0xa7fb, 0xa805, 0xa80b, 0xa81d, 0xa829, 0xa82b, 0xa837, 0xa83b, 0xa855, 0xa85f, 0xa86d, 0xa87d, 0xa88f, 0xa897, 0xa8a9, 0xa8b5, 0xa8c1, 0xa8c7, 0xa8d7, 0xa8e5, 0xa8fd, 0xa907, 0xa913, 0xa91b, 0xa931, 0xa937, 0xa939, 0xa943, 0xa97f, 0xa985, 0xa987, 0xa98b,
	0xa993, 0xa9a3, 0xa9b1, 0xa9bb, 0xa9c1, 0xa9d9, 0xa9df, 0xa9eb, 0xa9fd, 0xaa15, 0xaa17, 0xaa35, 0xaa39, 0xaa3b, 0xaa47, 0xaa4d, 0xaa57, 0xaa59, 0xaa5d, 0xaa6b, 0xaa71, 0xaa81, 0xaa83, 0xaa8d, 0xaa95, 0xaaab, 0xaabf, 0xaac5, 0xaac9, 0xaae9, 0xaaef, 0xab01,
	0xab05, 0xab07, 0xab0b, 0xab0d, 0xab11, 0xab19, 0xab4d, 0xab5b, 0xab71, 0xab73, 0xab89, 0xab9d, 0xaba7, 0xabaf, 0xabb9, 0xabbb, 0xabc1, 0xabc5, 0xabd3, 0xabd7, 0xabdd, 0xabf1, 0xabf5, 0xabfb, 0xabfd, 0xac09, 0xac15, 0xac1b, 0xac27, 0xac37, 0xac39, 0xac45,
	0xac4f, 0xac57, 0xac5b, 0xac61, 0xac63, 0xac7f, 0xac8b, 0xac93, 0xac9d, 0xaca9, 0xacab, 0xacaf, 0xacbd, 0xacd9, 0xace1, 0xace7, 0xaceb, 0xaced, 0xacf1, 0xacf7, 0xacf9, 0xad05, 0xad3f, 0xad45, 0xad53, 0xad5d, 0xad5f, 0xad65, 0xad81, 0xada1, 0xada5, 0xadc3,
	0xadcb, 0xadd1, 0xadd5, 0xaddb, 0xade7, 0xadf3, 0xadf5, 0xadf9, 0xadff, 0xae05, 0xae13, 0xae23, 0xae2b, 0xae49, 0xae4d, 0xae4f, 0xae59, 0xae61, 0xae67, 0xae6b, 0xae71, 0xae8b, 0xae8f, 0xae9b, 0xae9d, 0xaea7, 0xaeb9, 0xaec5, 0xaed1, 0xaee3, 0xaee5, 0xaee9,
	0xaef5, 0xaefd, 0xaf09, 0xaf13, 0xaf27, 0xaf2b, 0xaf33, 0xaf43, 0xaf4f, 0xaf57, 0xaf5d, 0xaf6d, 0xaf75, 0xaf7f, 0xaf8b, 0xaf99, 0xaf9f, 0xafa3, 0xafab, 0xafb7, 0xafbb, 0xafcf, 0xafd5, 0xaffd, 0xb005, 0xb015, 0xb01b, 0xb03f, 0xb041, 0xb047, 0xb04b, 0xb051,
	0xb053, 0xb069, 0xb07b, 0xb07d, 0xb087, 0xb08d, 0xb0b1, 0xb0bf, 0xb0cb, 0xb0cf, 0xb0e1, 0xb0e9, 0xb0ed, 0xb0fb, 0xb105, 0xb107, 0xb111, 0xb119, 0xb11d, 0xb11f, 0xb131, 0xb141, 0xb14d, 0xb15b, 0xb165, 0xb173, 0xb179, 0xb17f, 0xb1a9, 0xb1b3, 0xb1b9, 0xb1bf,
	0xb1d3, 0xb1dd, 0xb1e5, 0xb1f1, 0xb1f5, 0xb201, 0xb213, 0xb215, 0xb21f, 0xb22d, 0xb23f, 0xb249, 0xb25b, 0xb263, 0xb269, 0xb26d, 0xb27b, 0xb281, 0xb28b, 0xb2a9, 0xb2b7, 0xb2bd, 0xb2c3, 0xb2c7, 0xb2d3, 0xb2f9, 0xb2fd, 0xb2ff, 0xb303, 0xb309, 0xb311, 0xb31d,
	0xb327, 0xb32d, 0xb33f, 0xb345, 0xb377, 0xb37d, 0xb381, 0xb387, 0xb393, 0xb39b, 0xb3a5, 0xb3c5, 0xb3cb, 0xb3e1, 0xb3e3, 0xb3ed, 0xb3f9, 0xb40b, 0xb40d, 0xb413, 0xb417, 0xb435, 0xb43d, 0xb443, 0xb449, 0xb45b, 0xb465, 0xb467, 0xb46b, 0xb477, 0xb48b, 0xb495,
	0xb49d, 0xb4b5, 0xb4bf, 0xb4c1, 0xb4c7, 0xb4dd, 0xb4e3, 0xb4e5, 0xb4f7, 0xb501, 0xb50d, 0xb50f, 0xb52d, 0xb53f, 0xb54b, 0xb567, 0xb569, 0xb56f, 0xb573, 0xb579, 0xb587, 0xb58d, 0xb599, 0xb5a3, 0xb5ab, 0xb5af, 0xb5bb, 0xb5d5, 0xb5df, 0xb5e7, 0xb5ed, 0xb5fd,
	0xb5ff, 0xb609, 0xb61b, 0xb629, 0xb62f, 0xb633, 0xb639, 0xb647, 0xb657, 0xb659, 0xb65f, 0xb663, 0xb66f, 0xb683, 0xb687, 0xb69b, 0xb69f, 0xb6a5, 0xb6b1, 0xb6b3, 0xb6d7, 0xb6db, 0xb6e1, 0xb6e3, 0xb6ed, 0xb6ef, 0xb705, 0xb70d, 0xb713, 0xb71d, 0xb729, 0xb735,
	0xb747, 0xb755, 0xb76d, 0xb791, 0xb795, 0xb7a9, 0xb7c1, 0xb7cb, 0xb7d1, 0xb7d3, 0xb7ef, 0xb7f5, 0xb807, 0xb80f, 0xb813, 0xb819, 0xb821, 0xb827, 0xb82b, 0xb82d, 0xb839, 0xb855, 0xb867, 0xb875, 0xb885, 0xb893, 0xb8a5, 0xb8af, 0xb8b7, 0xb8bd, 0xb8c1, 0xb8c7,
	0xb8cd, 0xb8d5, 0xb8eb, 0xb8f7, 0xb8f9, 0xb903, 0xb915, 0xb91b, 0xb91d, 0xb92f, 0xb939, 0xb93b, 0xb947, 0xb951, 0xb963, 0xb983, 0xb989, 0xb98d, 0xb993, 0xb999, 0xb9a1, 0xb9a7, 0xb9ad, 0xb9b7, 0xb9cb, 0xb9d1, 0xb9dd, 0xb9e7, 0xb9ef, 0xb9f9, 0xba07, 0xba0d,
	0xba17, 0xba25, 0xba29, 0xba2b, 0xba41, 0xba53, 0xba55, 0xba5f, 0xba61, 0xba65, 0xba79, 0xba7d, 0xba7f, 0xbaa1, 0xbaa3, 0xbaaf, 0xbab5, 0xbabf, 0xbac1, 0xbacb, 0xbadd, 0xbae3, 0xbaf1, 0xbafd, 0xbb09, 0xbb1f, 0xbb27, 0xbb2d, 0xbb3d, 0xbb43, 0xbb4b, 0xbb4f,
	0xbb5b, 0xbb61, 0xbb69, 0xbb6d, 0xbb91, 0xbb97, 0xbb9d, 0xbbb1, 0xbbc9, 0xbbcf, 0xbbdb, 0xbbed, 0xbbf7, 0xbbf9, 0xbc03, 0xbc1d, 0xbc23, 0xbc33, 0xbc3b, 0xbc41, 0xbc45, 0xbc5d, 0xbc6f, 0xbc77, 0xbc83, 0xbc8f, 0xbc99, 0xbcab, 0xbcb7, 0xbcb9, 0xbcd1, 0xbcd5,
	0xbce1, 0xbcf3, 0xbcff, 0xbd0d, 0xbd17, 0xbd19, 0xbd1d, 0xbd35, 0xbd41, 0xbd4f, 0xbd59, 0xbd5f, 0xbd61, 0xbd67, 0xbd6b, 0xbd71, 0xbd8b, 0xbd8f, 0xbd95, 0xbd9b, 0xbd9d, 0xbdb3, 0xbdbb, 0xbdcd, 0xbdd1, 0xbde3, 0xbdeb, 0xbdef, 0xbe07, 0xbe09, 0xbe15, 0xbe21,
	0xbe25, 0xbe27, 0xbe5b, 0xbe5d, 0xbe6f, 0xbe75, 0xbe79, 0xbe7f, 0xbe8b, 0xbe8d, 0xbe93, 0xbe9f, 0xbea9, 0xbeb1, 0xbeb5, 0xbeb7, 0xbecf, 0xbed9, 0xbedb, 0xbee5, 0xbee7, 0xbef3, 0xbef9, 0xbf0b, 0xbf33, 0xbf39, 0xbf4d, 0xbf5d, 0xbf5f, 0xbf6b, 0xbf71, 0xbf7b,
	0xbf87, 0xbf89, 0xbf8d, 0xbf93, 0xbfa1, 0xbfad, 0xbfb9, 0xbfcf, 0xbfd5, 0xbfdd, 0xbfe1, 0xbfe3, 0xbff3, 0xc005, 0xc011, 0xc013, 0xc019, 0xc029, 0xc02f, 0xc031, 0xc037, 0xc03b, 0xc047, 0xc065, 0xc06d, 0xc07d, 0xc07f, 0xc091, 0xc09b, 0xc0b3, 0xc0b5, 0xc0bb,
	0xc0d3, 0xc0d7, 0xc0d9, 0xc0ef, 0xc0f1, 0xc101, 0xc103, 0xc109, 0xc115, 0xc119, 0xc12b, 0xc133, 0xc137, 0xc145, 0xc149, 0xc15b, 0xc173, 0xc179, 0xc17b, 0xc181, 0xc18b, 0xc18d, 0xc197, 0xc1bd, 0xc1c3, 0xc1cd, 0xc1db, 0xc1e1, 0xc1e7, 0xc1ff, 0xc203, 0xc205,
	0xc211, 0xc221, 0xc22f, 0xc23f, 0xc24b, 0xc24d, 0xc253, 0xc25d, 0xc277, 0xc27b, 0xc27d, 0xc289, 0xc28f, 0xc293, 0xc29f, 0xc2a7, 0xc2b3, 0xc2bd, 0xc2cf, 0xc2d5, 0xc2e3, 0xc2ff, 0xc301, 0xc307, 0xc311, 0xc313, 0xc317, 0xc325, 0xc347, 0xc349, 0xc34f, 0xc365,
	0xc367, 0xc371, 0xc37f, 0xc383, 0xc385, 0xc395, 0xc39d, 0xc3a7, 0xc3ad, 0xc3b5, 0xc3bf, 0xc3c7, 0xc3cb, 0xc3d1, 0xc3d3, 0xc3e3, 0xc3e9, 0xc3ef, 0xc401, 0xc41f, 0xc42d, 0xc433, 0xc437, 0xc455, 0xc457, 0xc461, 0xc46f, 0xc473, 0xc487, 0xc491, 0xc499, 0xc49d,
	0xc4a5, 0xc4b7, 0xc4bb, 0xc4c9, 0xc4cf, 0xc4d3, 0xc4eb, 0xc4f1, 0xc4f7, 0xc509, 0xc51b, 0xc51d, 0xc541, 0xc547, 0xc551, 0xc55f, 0xc56b, 0xc56f, 0xc575, 0xc577, 0xc595, 0xc59b, 0xc59f, 0xc5a1, 0xc5a7, 0xc5c3, 0xc5d7, 0xc5db, 0xc5ef, 0xc5fb, 0xc613, 0xc623,
	0xc635, 0xc641, 0xc64f, 0xc655, 0xc659, 0xc665, 0xc685, 0xc691, 0xc697, 0xc6a1, 0xc6a9, 0xc6b3, 0xc6b9, 0xc6cb, 0xc6cd, 0xc6dd, 0xc6eb, 0xc6f1, 0xc707, 0xc70d, 0xc719, 0xc71b, 0xc72d, 0xc731, 0xc739, 0xc757, 0xc763, 0xc767, 0xc773, 0xc775, 0xc77f, 0xc7a5,
	0xc7bb, 0xc7bd, 0xc7c1, 0xc7cf, 0xc7d5, 0xc7e1, 0xc7f9, 0xc7fd, 0xc7ff, 0xc803, 0xc811, 0xc81d, 0xc827, 0xc829, 0xc839, 0xc83f, 0xc853, 0xc857, 0xc86b, 0xc881, 0xc88d, 0xc88f, 0xc893, 0xc895, 0xc8a1, 0xc8b7, 0xc8cf, 0xc8d5, 0xc8db, 0xc8dd, 0xc8e3, 0xc8e7,
	0xc8ed, 0xc8ef, 0xc8f9, 0xc905, 0xc911, 0xc917, 0xc919, 0xc91f, 0xc92f, 0xc937, 0xc93d, 0xc941, 0xc953, 0xc95f, 0xc96b, 0xc979, 0xc97d, 0xc989, 0xc98f, 0xc997, 0xc99d, 0xc9af, 0xc9b5, 0xc9bf, 0xc9cb, 0xc9d9, 0xc9df, 0xc9e3, 0xc9eb, 0xca01, 0xca07, 0xca09,
	0xca25, 0xca37, 0xca39, 0xca4b, 0xca55, 0xca5b, 0xca69, 0xca73, 0xca75, 0xca7f, 0xca8d, 0xca93, 0xca9d, 0xca9f, 0xcab5, 0xcabb, 0xcac3, 0xcac9, 0xcad9, 0xcae5, 0xcaed, 0xcb03, 0xcb05, 0xcb09, 0xcb17, 0xcb29, 0xcb35, 0xcb3b, 0xcb53, 0xcb59, 0xcb63, 0xcb65,
	0xcb71, 0xcb87, 0xcb99, 0xcb9f, 0xcbb3, 0xcbb9, 0xcbc3, 0xcbd1, 0xcbd5, 0xcbd7, 0xcbdd, 0xcbe9, 0xcbff, 0xcc0d, 0xcc19, 0xcc1d, 0xcc23, 0xcc2b, 0xcc41, 0xcc43, 0xcc4d, 0xcc59, 0xcc61, 0xcc89, 0xcc8b, 0xcc91, 0xcc9b, 0xcca3, 0xcca7, 0xccd1, 0xcce5, 0xcce9,
	0xcd09, 0xcd15, 0xcd1f, 0xcd25, 0xcd31, 0xcd3d, 0xcd3f, 0xcd49, 0xcd51, 0xcd57, 0xcd5b, 0xcd63, 0xcd67, 0xcd81, 0xcd93, 0xcd97, 0xcd9f, 0xcdbb, 0xcdc1, 0xcdd3, 0xcdd9, 0xcde5, 0xcde7, 0xcdf1, 0xcdf7, 0xcdfd, 0xce0b, 0xce15, 0xce21, 0xce2f, 0xce47, 0xce4d,
	0xce51, 0xce65, 0xce7b, 0xce7d, 0xce8f, 0xce93, 0xce99, 0xcea5, 0xcea7, 0xceb7, 0xcec9, 0xced7, 0xcedd, 0xcee3, 0xcee7, 0xceed, 0xcef5, 0xcf07, 0xcf0b, 0xcf19, 0xcf37, 0xcf3b, 0xcf4d, 0xcf55, 0xcf5f, 0xcf61, 0xcf65, 0xcf6d, 0xcf79, 0xcf7d, 0xcf89, 0xcf9b,
	0xcf9d, 0xcfa9, 0xcfb3, 0xcfb5, 0xcfc5, 0xcfcd, 0xcfd1, 0xcfef, 0xcff1, 0xcff7, 0xd013, 0xd015, 0xd01f, 0xd021, 0xd033, 0xd03d, 0xd04b, 0xd04f, 0xd069, 0xd06f, 0xd081, 0xd085, 0xd099, 0xd09f, 0xd0a3, 0xd0ab, 0xd0bd, 0xd0c1, 0xd0cd, 0xd0e7, 0xd0ff, 0xd103,
	0xd117, 0xd12d, 0xd12f, 0xd141, 0xd157, 0xd159, 0xd15d, 0xd169, 0xd16b, 0xd171, 0xd177, 0xd17d, 0xd181, 0xd187, 0xd195, 0xd199, 0xd1b1, 0xd1bd, 0xd1c3, 0xd1d5, 0xd1d7, 0xd1e3, 0xd1ff, 0xd20d, 0xd211, 0xd217, 0xd21f, 0xd235, 0xd23b, 0xd247, 0xd259, 0xd261,
	0xd265, 0xd279, 0xd27f, 0xd283, 0xd289, 0xd28b, 0xd29d, 0xd2a3, 0xd2a7, 0xd2b3, 0xd2bf, 0xd2c7, 0xd2e3, 0xd2e9, 0xd2f1, 0xd2fb, 0xd2fd, 0xd315, 0xd321, 0xd32b, 0xd343, 0xd34b, 0xd355, 0xd369, 0xd375, 0xd37b, 0xd387, 0xd393, 0xd397, 0xd3a5, 0xd3b1, 0xd3c9,
	0xd3eb, 0xd3fd, 0xd405, 0xd40f, 0xd415, 0xd427, 0xd42f, 0xd433, 0xd43b, 0xd44b, 0xd459, 0xd45f, 0xd463, 0xd469, 0xd481, 0xd483, 0xd489, 0xd48d, 0xd493, 0xd495, 0xd4a5, 0xd4ab, 0xd4b1, 0xd4c5, 0xd4dd, 0xd4e1, 0xd4e3, 0xd4e7, 0xd4f5, 0xd4f9, 0xd50b, 0xd50d,
	0xd513, 0xd51f, 0xd523, 0xd531, 0xd535, 0xd537, 0xd549, 0xd559, 0xd55f, 0xd565, 0xd567, 0xd577, 0xd58b, 0xd591, 0xd597, 0xd5b5, 0xd5b9, 0xd5c1, 0xd5c7, 0xd5df, 0xd5ef, 0xd5f5, 0xd5fb, 0xd603, 0xd60f, 0xd62d, 0xd631, 0xd643, 0xd655, 0xd65d, 0xd661, 0xd67b,
	0xd685, 0xd687, 0xd69d, 0xd6a5, 0xd6af, 0xd6bd, 0xd6c3, 0xd6c7, 0xd6d9, 0xd6e1, 0xd6ed, 0xd709, 0xd70b, 0xd711, 0xd715, 0xd721, 0xd727, 0xd73f, 0xd745, 0xd74d, 0xd757, 0xd76b, 0xd77b, 0xd783, 0xd7a1, 0xd7a7, 0xd7ad, 0xd7b1, 0xd7b3, 0xd7bd, 0xd7cb, 0xd7d1,
	0xd7db, 0xd7fb, 0xd811, 0xd823, 0xd825, 0xd829, 0xd82b, 0xd82f, 0xd837, 0xd84d, 0xd855, 0xd867, 0xd873, 0xd88f, 0xd891, 0xd8a1, 0xd8ad, 0xd8bf, 0xd8cd, 0xd8d7, 0xd8e9, 0xd8f5, 0xd8fb, 0xd91b, 0xd925, 0xd933, 0xd939, 0xd943, 0xd945, 0xd94f, 0xd951, 0xd957,
	0xd96d, 0xd96f, 0xd973, 0xd979, 0xd981, 0xd98b, 0xd991, 0xd99f, 0xd9a5, 0xd9a9, 0xd9b5, 0xd9d3, 0xd9eb, 0xd9f1, 0xd9f7, 0xd9ff, 0xda05, 0xda09, 0xda0b, 0xda0f, 0xda15, 0xda1d, 0xda23, 0xda29, 0xda3f, 0xda51, 0xda59, 0xda5d, 0xda5f, 0xda71, 0xda77, 0xda7b,
	0xda7d, 0xda8d, 0xda9f, 0xdab3, 0xdabd, 0xdac3, 0xdac9, 0xdae7, 0xdae9, 0xdaf5, 0xdb11, 0xdb17, 0xdb1d, 0xdb23, 0xdb25, 0xdb31, 0xdb3b, 0xdb43, 0xdb55, 0xdb67, 0xdb6b, 0xdb73, 0xdb85, 0xdb8f, 0xdb91, 0xdbad, 0xdbaf, 0xdbb9, 0xdbc7, 0xdbcb, 0xdbcd, 0xdbeb,
	0xdbf7, 0xdc0d, 0xdc27, 0xdc31, 0xdc39, 0xdc3f, 0xdc49, 0xdc51, 0xdc61, 0xdc6f, 0xdc75, 0xdc7b, 0xdc85, 0xdc93, 0xdc99, 0xdc9d, 0xdc9f, 0xdca9, 0xdcb5, 0xdcb7, 0xdcbd, 0xdcc7, 0xdccf, 0xdcd3, 0xdcd5, 0xdcdf, 0xdcf9, 0xdd0f, 0xdd15, 0xdd17, 0xdd23, 0xdd35,
	0xdd39, 0xdd53, 0xdd57, 0xdd5f, 0xdd69, 0xdd6f, 0xdd7d, 0xdd87, 0xdd89, 0xdd9b, 0xdda1, 0xddab, 0xddbf, 0xddc5, 0xddcb, 0xddcf, 0xdde7, 0xdde9, 0xdded, 0xddf5, 0xddfb, 0xde0b, 0xde19, 0xde29, 0xde3b, 0xde3d, 0xde41, 0xde4d, 0xde4f, 0xde59, 0xde5b, 0xde61,
	0xde6d, 0xde77, 0xde7d, 0xde83, 0xde97, 0xde9d, 0xdea1, 0xdea7, 0xdecd, 0xded1, 0xded7, 0xdee3, 0xdef1, 0xdef5, 0xdf01, 0xdf09, 0xdf13, 0xdf1f, 0xdf2b, 0xdf33, 0xdf37, 0xdf3d, 0xdf4b, 0xdf55, 0xdf5b, 0xdf67, 0xdf69, 0xdf73, 0xdf85, 0xdf87, 0xdf99, 0xdfa3,
	0xdfab, 0xdfb5, 0xdfb7, 0xdfc3, 0xdfc7, 0xdfd5, 0xdff1, 0xdff3, 0xe003, 0xe005, 0xe017, 0xe01d, 0xe027, 0xe02d, 0xe035, 0xe045, 0xe053, 0xe071, 0xe07b, 0xe08f, 0xe095, 0xe09f, 0xe0b7, 0xe0b9, 0xe0d5, 0xe0d7, 0xe0e3, 0xe0f3, 0xe0f9, 0xe101, 0xe125, 0xe129,
	0xe131, 0xe135, 0xe143, 0xe14f, 0xe159, 0xe161, 0xe16d, 0xe171, 0xe177, 0xe17f, 0xe183, 0xe189, 0xe197, 0xe1ad, 0xe1b5, 0xe1bb, 0xe1bf, 0xe1c1, 0xe1cb, 0xe1d1, 0xe1e5, 0xe1ef, 0xe1f7, 0xe1fd, 0xe203, 0xe219, 0xe22b, 0xe22d, 0xe23d, 0xe243, 0xe257, 0xe25b,
	0xe275, 0xe279, 0xe287, 0xe29d, 0xe2ab, 0xe2af, 0xe2bb, 0xe2c1, 0xe2c9, 0xe2cd, 0xe2d3, 0xe2d9, 0xe2f3, 0xe2fd, 0xe2ff, 0xe311, 0xe323, 0xe327, 0xe329, 0xe339, 0xe33b, 0xe34d, 0xe351, 0xe357, 0xe35f, 0xe363, 0xe369, 0xe375, 0xe377, 0xe37d, 0xe383, 0xe39f,
	0xe3c5, 0xe3c9, 0xe3d1, 0xe3e1, 0xe3fb, 0xe3ff, 0xe401, 0xe40b, 0xe417, 0xe419, 0xe423, 0xe42b, 0xe431, 0xe43b, 0xe447, 0xe449, 0xe453, 0xe455, 0xe46d, 0xe471, 0xe48f, 0xe4a9, 0xe4af, 0xe4b5, 0xe4c7, 0xe4cd, 0xe4d3, 0xe4e9, 0xe4eb, 0xe4f5, 0xe507, 0xe521,
	0xe525, 0xe537, 0xe53f, 0xe545, 0xe54b, 0xe557, 0xe567, 0xe56d, 0xe575, 0xe585, 0xe58b, 0xe593, 0xe5a3, 0xe5a5, 0xe5cf, 0xe609, 0xe611, 0xe615, 0xe61b, 0xe61d, 0xe621, 0xe629, 0xe639, 0xe63f, 0xe653, 0xe657, 0xe663, 0xe66f, 0xe675, 0xe681, 0xe683, 0xe68d,
	0xe68f, 0xe695, 0xe6ab, 0xe6ad, 0xe6b7, 0xe6bd, 0xe6c5, 0xe6cb, 0xe6d5, 0xe6e3, 0xe6e9, 0xe6ef, 0xe6f3, 0xe705, 0xe70d, 0xe717, 0xe71f, 0xe72f, 0xe73d, 0xe747, 0xe749, 0xe753, 0xe755, 0xe761, 0xe767, 0xe76b, 0xe77f, 0xe789, 0xe791, 0xe7c5, 0xe7cd, 0xe7d7,
	0xe7dd, 0xe7df, 0xe7e9, 0xe7f1, 0xe7fb, 0xe801, 0xe807, 0xe80f, 0xe819, 0xe81b, 0xe831, 0xe833, 0xe837, 0xe83d, 0xe84b, 0xe84f, 0xe851, 0xe869, 0xe875, 0xe879, 0xe893, 0xe8a5, 0xe8a9, 0xe8af, 0xe8bd, 0xe8db, 0xe8e1, 0xe8e5, 0xe8eb, 0xe8ed, 0xe903, 0xe90b,
	0xe90f, 0xe915, 0xe917, 0xe92d, 0xe933, 0xe93b, 0xe94b, 0xe951, 0xe95f, 0xe963, 0xe969, 0xe97b, 0xe983, 0xe98f, 0xe995, 0xe9a1, 0xe9b9, 0xe9d7, 0xe9e7, 0xe9ef, 0xea11, 0xea19, 0xea2f, 0xea35, 0xea43, 0xea4d, 0xea5f, 0xea6d, 0xea71, 0xea7d, 0xea85, 0xea89,
	0xeaad, 0xeab3, 0xeab9, 0xeabb, 0xeac5, 0xeac7, 0xeacb, 0xeadf, 0xeae5, 0xeaeb, 0xeaf5, 0xeb01, 0xeb07, 0xeb09, 0xeb31, 0xeb39, 0xeb3f, 0xeb5b, 0xeb61, 0xeb63, 0xeb6f, 0xeb81, 0xeb85, 0xeb9d, 0xebab, 0xebb1, 0xebb7, 0xebc1, 0xebd5, 0xebdf, 0xebed, 0xebfd,
	0xec0b, 0xec1b, 0xec21, 0xec29, 0xec4d, 0xec51, 0xec5d, 0xec69, 0xec6f, 0xec7b, 0xecad, 0xecb9, 0xecbf, 0xecc3, 0xecc9, 0xeccf, 0xecd7, 0xecdd, 0xece7, 0xece9, 0xecf3, 0xecf5, 0xed07, 0xed11, 0xed1f, 0xed2f, 0xed37, 0xed3d, 0xed41, 0xed55, 0xed59, 0xed5b,
	0xed65, 0xed6b, 0xed79, 0xed8b, 0xed95, 0xedbb, 0xedc5, 0xedd7, 0xedd9, 0xede3, 0xede5, 0xedf1, 0xedf5, 0xedf7, 0xedfb, 0xee09, 0xee0f, 0xee19, 0xee21, 0xee49, 0xee4f, 0xee63, 0xee67, 0xee73, 0xee7b, 0xee81, 0xeea3, 0xeeab, 0xeec1, 0xeec9, 0xeed5, 0xeedf,
	0xeee1, 0xeef1, 0xef1b, 0xef27, 0xef2f, 0xef45, 0xef4d, 0xef63, 0xef6b, 0xef71, 0xef93, 0xef95, 0xef9b, 0xef9f, 0xefad, 0xefb3, 0xefc3, 0xefc5, 0xefdb, 0xefe1, 0xefe9, 0xf001, 0xf017, 0xf01d, 0xf01f, 0xf02b, 0xf02f, 0xf035, 0xf043, 0xf047, 0xf04f, 0xf067,
	0xf06b, 0xf071, 0xf077, 0xf079, 0xf08f, 0xf0a3, 0xf0a9, 0xf0ad, 0xf0bb, 0xf0bf, 0xf0c5, 0xf0cb, 0xf0d3, 0xf0d9, 0xf0e3, 0xf0e9, 0xf0f1, 0xf0f7, 0xf107, 0xf115, 0xf11b, 0xf121, 0xf137, 0xf13d, 0xf155, 0xf175, 0xf17b, 0xf18d, 0xf193, 0xf1a5, 0xf1af, 0xf1b7,
	0xf1d5, 0xf1e7, 0xf1ed, 0xf1fd, 0xf209, 0xf20f, 0xf21b, 0xf21d, 0xf223, 0xf227, 0xf233, 0xf23b, 0xf241, 0xf257, 0xf25f, 0xf265, 0xf269, 0xf277, 0xf281, 0xf293, 0xf2a7, 0xf2b1, 0xf2b3, 0xf2b9, 0xf2bd, 0xf2bf, 0xf2db, 0xf2ed, 0xf2ef, 0xf2f9, 0xf2ff, 0xf305,
	0xf30b, 0xf319, 0xf341, 0xf359, 0xf35b, 0xf35f, 0xf367, 0xf373, 0xf377, 0xf38b, 0xf38f, 0xf3af, 0xf3c1, 0xf3d1, 0xf3d7, 0xf3fb, 0xf403, 0xf409, 0xf40d, 0xf413, 0xf421, 0xf425, 0xf42b, 0xf445, 0xf44b, 0xf455, 0xf463, 0xf475, 0xf47f, 0xf485, 0xf48b, 0xf499,
	0xf4a3, 0xf4a9, 0xf4af, 0xf4bd, 0xf4c3, 0xf4db, 0xf4df, 0xf4ed, 0xf503, 0xf50b, 0xf517, 0xf521, 0xf529, 0xf535, 0xf547, 0xf551, 0xf563, 0xf56b, 0xf583, 0xf58d, 0xf595, 0xf599, 0xf5b1, 0xf5b7, 0xf5c9, 0xf5cf, 0xf5d1, 0xf5db, 0xf5f9, 0xf5fb, 0xf605, 0xf607,
	0xf60b, 0xf60d, 0xf635, 0xf637, 0xf653, 0xf65b, 0xf661, 0xf667, 0xf679, 0xf67f, 0xf689, 0xf697, 0xf69b, 0xf6ad, 0xf6cb, 0xf6dd, 0xf6df, 0xf6eb, 0xf709, 0xf70f, 0xf72d, 0xf731, 0xf743, 0xf74f, 0xf751, 0xf755, 0xf763, 0xf769, 0xf773, 0xf779, 0xf781, 0xf787,
	0xf791, 0xf79d, 0xf79f, 0xf7a5, 0xf7b1, 0xf7bb, 0xf7bd, 0xf7cf, 0xf7d3, 0xf7e7, 0xf7eb, 0xf7f1, 0xf7ff, 0xf805, 0xf80b, 0xf821, 0xf827, 0xf82d, 0xf835, 0xf847, 0xf859, 0xf863, 0xf865, 0xf86f, 0xf871, 0xf877, 0xf87b, 0xf881, 0xf88d, 0xf89f, 0xf8a1, 0xf8ab,
	0xf8b3, 0xf8b7, 0xf8c9, 0xf8cb, 0xf8d1, 0xf8d7, 0xf8dd, 0xf8e7, 0xf8ef, 0xf8f9, 0xf8ff, 0xf911, 0xf91d, 0xf925, 0xf931, 0xf937, 0xf93b, 0xf941, 0xf94f, 0xf95f, 0xf961, 0xf96d, 0xf971, 0xf977, 0xf99d, 0xf9a3, 0xf9a9, 0xf9b9, 0xf9cd, 0xf9e9, 0xf9fd, 0xfa07,
	0xfa0d, 0xfa13, 0xfa21, 0xfa25, 0xfa3f, 0xfa43, 0xfa51, 0xfa5b, 0xfa6d, 0xfa7b, 0xfa97, 0xfa99, 0xfa9d, 0xfaab, 0xfabb, 0xfabd, 0xfad9, 0xfadf, 0xfae7, 0xfaed, 0xfb0f, 0xfb17, 0xfb1b, 0xfb2d, 0xfb2f, 0xfb3f, 0xfb47, 0xfb4d, 0xfb75, 0xfb7d, 0xfb8f, 0xfb93,
	0xfbb1, 0xfbb7, 0xfbc3, 0xfbc5, 0xfbe3, 0xfbe9, 0xfbf3, 0xfc01, 0xfc29, 0xfc37, 0xfc41, 0xfc43, 0xfc4f, 0xfc59, 0xfc61, 0xfc65, 0xfc6d, 0xfc73, 0xfc79, 0xfc95, 0xfc97, 0xfc9b, 0xfca7, 0xfcb5, 0xfcc5, 0xfccd, 0xfceb, 0xfcfb, 0xfd0d, 0xfd0f, 0xfd19, 0xfd2b,
	0xfd31, 0xfd51, 0xfd55, 0xfd67, 0xfd6d, 0xfd6f, 0xfd7b, 0xfd85, 0xfd97, 0xfd99, 0xfd9f, 0xfda9, 0xfdb7, 0xfdc9, 0xfde5, 0xfdeb, 0xfdf3, 0xfe03, 0xfe05, 0xfe09, 0xfe1d, 0xfe27, 0xfe2f, 0xfe41, 0xfe4b, 0xfe4d, 0xfe57, 0xfe5f, 0xfe63, 0xfe69, 0xfe75, 0xfe7b,
	0xfe8f, 0xfe93, 0xfe95, 0xfe9b, 0xfe9f, 0xfeb3, 0xfebd, 0xfed7, 0xfee9, 0xfef3, 0xfef5, 0xff07, 0xff0d, 0xff1d, 0xff2b, 0xff2f, 0xff49, 0xff4d, 0xff5b, 0xff65, 0xff71, 0xff7f, 0xff85, 0xff8b, 0xff8f, 0xff9d, 0xffa7, 0xffa9, 0xffc7, 0xffd9, 0xffef
};

void leftRotateArray(uint8_t array[], size_t arraySize, uint16_t bits);

int main(int argc, char **argv) {

	size_t pLen;
	uint8_t p[STDIN_BUFFER_SIZE], *t;
	uint8_t state[CHYMOS_OUTPUT_BYTES];
	uint8_t rowState[CHYMOS_OUTPUT_BYTES];
	uint8_t primeFirstChunk, primeLastChunk;

	uint16_t i;
	unsigned int j, k;

	const uint8_t *sha3_hash;

	WHIRLPOOL_CTX whirlpool_CTX;
	sha3_context sha3_CTX;
	GOST34112012Context *streebog_CTX;

	memset(p, 0x00, STDIN_BUFFER_SIZE);
	pLen = fread(p, (size_t)1, (size_t)STDIN_BUFFER_SIZE, stdin);
	t = malloc(pLen);
	memcpy(t, p, pLen);

	memset(rowState,       0x00, CHYMOS_OUTPUT_BYTES);
	memset(state,          0x00, CHYMOS_OUTPUT_BYTES);

	streebog_CTX = malloc(sizeof(GOST34112012Context));

	for (j = 0; j < 2048; j++) {

		if (j > 0) {
			primeFirstChunk = (thetas[j-1] >> 8);
			primeLastChunk = thetas[j-1];
			for (k = 0; k < pLen; k++) {
				if ((k % 2) == 0) {
					t[k] = (p[k] ^ primeFirstChunk);
				} else {
					t[k] = (p[k] ^ primeLastChunk);
				}
			}
		}

		///////////////////////////////////////////// WHIRLPOOL /////////////////////////////////////////////

		WHIRLPOOL_Init(&whirlpool_CTX);
		WHIRLPOOL_Update(&whirlpool_CTX, t, pLen);
		WHIRLPOOL_Final(rowState, &whirlpool_CTX);

		///////////////////////////////////////////// SHA-2-512 /////////////////////////////////////////////

		SHA512(t, pLen, (rowState+64));

		///////////////////////////////////////////// SHA-3-512 /////////////////////////////////////////////

		sha3_Init512(&sha3_CTX);
		sha3_Update(&sha3_CTX, t, pLen);
		sha3_hash = sha3_Finalize(&sha3_CTX);
		memcpy((rowState+128), sha3_hash, 64);

		///////////////////////////////////////////// Streebog-512 /////////////////////////////////////////////
		// https://tools.ietf.org/html/rfc6986 , https://www.streebog.net/usage.txt

		GOST34112012Init(streebog_CTX, 512);
		GOST34112012Update(streebog_CTX, &t[0], pLen);
		GOST34112012Final(streebog_CTX, &rowState[192]);
		GOST34112012Cleanup(streebog_CTX);

		////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Left-Rotate rowState
		//void leftRotateArray(uint8_t array[], size_t arraySize, uint16_t bits);
		if (j > 0) {
			leftRotateArray(&rowState[0], CHYMOS_OUTPUT_BYTES, j);
		}

		// and mix rowState into state
		for (i = 0; i < CHYMOS_OUTPUT_BYTES; i++) {
			state[i] ^= rowState[i];
		}
	}

	// Output the final digest in hex
	for (i = 0; i < CHYMOS_OUTPUT_BYTES; i++) {
		printf("%02x", state[i]);
	}
	printf("\n");

	return EXIT_SUCCESS;
}

/*
 * n.b. a nice patch for this would likely be appreciated.
 * Either e-mail to:
 *   charlesmorris@gmail.com, cc: cmorris@cs.odu.edu
 * or pull request on github:
 *   https://github.com/apaxmai/CHMk-1-2048
 */
void leftRotateArray(uint8_t array[], size_t arraySize, uint16_t bits)
{
        uint8_t partialAmount, *t, mask, firstByteOverflow, byteOverflow;
        uint16_t completeBytes;
        uint16_t i;

        uint8_t n, j;

        partialAmount = (bits % 8);
        completeBytes = floor(bits / 8);

        t = malloc(completeBytes);

        memcpy(t, &array[0], completeBytes);
        memmove(&array[0], (&array[completeBytes]), (arraySize-completeBytes));
        memcpy((&array[arraySize - completeBytes]), t, completeBytes);

        if (partialAmount != 0) {
		mask = (0xFF >> partialAmount);
		firstByteOverflow = array[0] << (8 - partialAmount);
		array[0] >>= partialAmount;

		for (i = 1; i < arraySize; i++) {
			byteOverflow = array[i] << (8 - partialAmount);
			array[i] >>= partialAmount;
			array[i - 1] = (array[i - 1] & mask) | byteOverflow;
		}

		array[arraySize-1] |= firstByteOverflow;
	}

	free(t);
}



