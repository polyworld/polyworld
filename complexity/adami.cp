#include "adami.h"

#include <stdio.h>

#include "GenomeUtil.h"
#include "objectxsortedlist.h"
#include "Simulation.h"

using namespace genome;


void computeAdamiComplexity( long timestep,
							 FILE *FileOneBit,
							 FILE *FileTwoBit,
							 FILE *FileFourBit,
							 FILE *FileSummary )
{
	unsigned int genevalue;

	float SumInformationOneBit = 0;
	float SumInformationTwoBit = 0;
	float SumInformationFourBit = 0;
			
	float entropyOneBit[8];
	float informationOneBit[8];
			
	float entropyTwoBit[4];
	float informationTwoBit[4];
	float entropyFourBit[2];
	float informationFourBit[2];
						
	agent* c = NULL;

	if( ftell(FileOneBit) == 0 )
	{
		fprintf( FileOneBit, "%% BitsInGenome: %d WindowSize: 1\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fprintf( FileTwoBit, "%% BitsInGenome: %d WindowSize: 2\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fprintf( FileFourBit, "%% BitsInGenome: %d WindowSize: 4\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fprintf( FileSummary, "%% Timestep 1bit 2bit 4bit\n" );
	}


	fprintf( FileOneBit,  "%ld:", timestep );		// write the timestep on the beginning of the line
	fprintf( FileTwoBit,  "%ld:", timestep );		// write the timestep on the beginning of the line
	fprintf( FileFourBit, "%ld:", timestep );		// write the timestep on the beginning of the line
	fprintf( FileSummary, "%ld ", timestep );		// write the timestep on the beginning of the line
			
	int numagents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);

	bool bits[numagents][8];
		
	for( int gene = 0, n = GenomeUtil::schema->getMutableSize(); gene < n; gene++ )			// for each gene ...
	{
		int count = 0;

		objectxsortedlist::gXSortedObjects.reset();				
		
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )	// for each agent ...
		{
			genevalue = c->Genes()->get_raw_uint(gene);

					
			if( genevalue >= 128 ) { bits[count][0]=true; genevalue -= 128; } else { bits[count][0] = false; }
			if( genevalue >= 64  ) { bits[count][1]=true; genevalue -= 64; }  else { bits[count][1] = false; }
			if( genevalue >= 32  ) { bits[count][2]=true; genevalue -= 32; }  else { bits[count][2] = false; }
			if( genevalue >= 16  ) { bits[count][3]=true; genevalue -= 16; }  else { bits[count][3] = false; }
			if( genevalue >= 8   ) { bits[count][4]=true; genevalue -= 8; }   else { bits[count][4] = false; }
			if( genevalue >= 4   ) { bits[count][5]=true; genevalue -= 4; }   else { bits[count][5] = false; }
			if( genevalue >= 2   ) { bits[count][6]=true; genevalue -= 2; }   else { bits[count][6] = false; }
			if( genevalue == 1   ) { bits[count][7]=true; }                   else { bits[count][7] = false; }

			count++;
		}
				
				
		/* PRINT THE BYTE UNDER CONSIDERATION */
				
		/* DOING ONE BIT WINDOW */
		for( int i=0; i<8; i++ )		// for each window 1-bits wide...
		{
			int number_of_ones=0;
					
			for( int agent=0; agent<numagents; agent++ )
				if( bits[agent][i] == true ) { number_of_ones++; }		// if agent has a 1 in the column, increment number_of_ones
										
			float prob_1 = (float) number_of_ones / (float) numagents;
			float prob_0 = 1.0 - prob_1;
			float logprob_0, logprob_1;
					
			if( prob_1 == 0.0 ) logprob_1 = 0.0;
			else logprob_1 = log2(prob_1);

			if( prob_0 == 0.0 ) logprob_0 = 0.0;
			else logprob_0 = log2(prob_0);

					
			entropyOneBit[i] = -1 * ( (prob_0 * logprob_0) + (prob_1 * logprob_1) );			// calculating the entropy for this bit...
			informationOneBit[i] = 1.0 - entropyOneBit[i];										// 1.0 = maxentropy
			SumInformationOneBit += informationOneBit[i];
			//DEBUG				cout << "Gene" << gene << "_bit[" << i << "]: probs =\t{" << prob_0 << "," << prob_1 << "}\tEntropy =\t" << entropyOneBit[i] << " bits\t\tInformation =\t" << informationOneBit[i] << endl;
		}
			
		fprintf( FileOneBit, " %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f", informationOneBit[0], informationOneBit[1], informationOneBit[2], informationOneBit[3], informationOneBit[4], 
				 informationOneBit[5], informationOneBit[6], informationOneBit[7] );

		/* DOING TWO BIT WINDOW */

		for( int i=0; i<4; i++ )		// for each window 2-bits wide...
		{
			int number_of[4];
			for( int j=0; j<4; j++) { number_of[j] = 0; }		// zero out the array

			for( int agent=0; agent<numagents; agent++ )
			{
				if( bits[agent][i*2] )			// Bits: 1 ?
				{
					if( bits[agent][(i*2)+1] ) { number_of[3]++; }	// Bits: 1 1
					else						 { number_of[2]++; }		// Bits: 1 0							
				}
				else							// Bits: 0 ?
				{
					if( bits[agent][(i*2)+1] ) { number_of[1]++; }		// Bits: 0 1
					else						 { number_of[0]++; }		// Bits: 0 0
				}
			}



			float prob[4];
			float logprob[4];
			float sum=0;
					
			for( int j=0; j<4; j++ )
			{
				prob[j] = (float) number_of[j] / (float) numagents;
				if( prob[j] == 0.0 ) { logprob[j] = 0.0; }
				else { logprob[j] = log2(prob[j]); }						// in units of mers

				sum += prob[j] * logprob[j];						// H(X) = -1 * SUM{ p(x) * log p(x) }
			}

			entropyTwoBit[i] = (sum * -1);							// multiplying the sum by -1 to get the Entropy
			informationTwoBit[i] = 2.0 - entropyTwoBit[i];			// since we're in units of mers, maxentroopy is always 1.0
			SumInformationTwoBit += informationTwoBit[i];

			//DEBUG				cout << "Gene" << gene << "_window2bit[" << i << "]: probs =\t{" << prob[0] << "," << prob[1] << "," << prob[2] << "," << prob[3] << "}\tEntropy =\t" << entropyTwoBit[i] << " mers\t\tInformation =\t" << informationTwoBit[i] << endl;
		}
				
		fprintf( FileTwoBit, " %.4f %.4f %.4f %.4f", informationTwoBit[0], informationTwoBit[1], informationTwoBit[2], informationTwoBit[3] );


		/* DOING FOUR BIT WINDOW */

		for( int i=0; i<2; i++ )		// for each window four-bits wide...
		{
			int number_of[16];
			for( int j=0; j<16; j++) { number_of[j] = 0; }		// zero out the array

					
			for( int agent=0; agent<numagents; agent++ )
			{
				if( bits[agent][i*4] )					// 1 ? ? ?.  Possibilities are 8-15
				{
					if( bits[agent][(i*4)+1] )			// 1 1 ? ?.  Possibilities are 12-15
					{
						if( bits[agent][(i*4)+2] )		// 1 1 1 ?.  Possibilities are 14-15
						{
							if( bits[agent][(i*4)+3] )	{ number_of[15]++; }	// 1 1 1 1
							else							{ number_of[14]++; }	// 1 1 1 0
						}
						else								// 1 1 0 ?.  Possibilities are 12-13
						{
							if( bits[agent][(i*4)+3] )	{ number_of[13]++; }	// 1 1 0 1
							else							{ number_of[12]++; }	// 1 1 0 0
						}
					}
					else									// 1 0 ? ?.  Possibilities are 8-11
					{
						if( bits[agent][(i*4)+2] )		// 1 0 1 ?.  Possibilities are 14-15
						{
							if( bits[agent][(i*4)+3] )	{ number_of[11]++; }	// 1 0 1 1
							else							{ number_of[10]++; }	// 1 0 1 0
						}
						else								// 1 0 0 ?.  Possibilities are 12-13
						{
							if( bits[agent][(i*4)+3] )	{ number_of[9]++; }		// 1 0 0 1
							else							{ number_of[8]++; }		// 1 0 0 0
						}
					}
				}
				else										// 0 ? ? ?.  Possibilities are 0-7
				{
					if( bits[agent][(i*4)+1] )			// 0 1 ? ?.  Possibilities are 4-8
					{
						if( bits[agent][(i*4)+2] )		// 0 1 1 ?.  Possibilities are 6-7
						{
							if( bits[agent][(i*4)+3] )	{ number_of[7]++; } // 0 1 1 1
							else							{ number_of[6]++; }	// 0 1 0 0
						}
						else								// 0 1 0 ?.  Possibilities are 4-5
						{
							if( bits[agent][(i*4)+3] )	{ number_of[5]++; } // 0 1 0 1
							else							{ number_of[4]++; }	// 0 1 0 0
						}
					}
					else									// 0 0 ? ?.  Possibilities are 0-3
					{
						if( bits[agent][(i*4)+2] )		// 0 0 1 ?.  Possibilities are 2-3
						{
							if( bits[agent][(i*4)+3] )	{ number_of[3]++; } // 0 0 1 1
							else							{ number_of[2]++; }	// 0 0 1 0
						}
						else								// 0 0 0 ?.  Possibilities are 0-1
						{
							if( bits[agent][(i*4)+3] )	{ number_of[1]++; } // 0 0 0 1
							else							{ number_of[0]++; }	// 0 0 0 0
						}							
					}
				}
			}
					
			float prob[16];
			float logprob[16];
			float sum=0;
					
			for( int j=0; j<16; j++ )
			{
				prob[j] = (float) number_of[j] / (float) numagents;
				if( prob[j] == 0.0 ) { logprob[j] = 0.0; }
				else { logprob[j] = log2(prob[j]); }

				sum += prob[j] * logprob[j];						// H(X) = -1 * SUM{ p(x) * log p(x) }
			}
					
			entropyFourBit[i] = (sum * -1);							// multiplying the sum by -1 to get the Entropy
			informationFourBit[i] = 4.0 - entropyFourBit[i];		// since we're in units of mers, maxentroopy is always 1.0
			SumInformationFourBit += informationFourBit[i];
			//DEBUG				cout << "Gene" << gene << "_window4bit[" << i << "]: probs =\t{" << prob[0] << "," << prob[1] << "," << prob[2] << "," << prob[3] << "," << prob[4] << "," << 
			//DEBUG					prob[5] << "," << prob[6] << "," << prob[7] << "," << prob[8] << "," << prob[9] << "," << prob[10] << "," << prob[11] << "," << 
			//DEBUG					prob[12] << "," << prob[13] << "," << prob[14] << "," << prob[15] << "}\tEntropy =\t" << entropyFourBit[i] << " mers\t\tInformation =\t" << informationFourBit[i] << endl;
		}

		fprintf( FileFourBit, " %.4f %.4f", informationFourBit[0], informationFourBit[1] );

	}

	fprintf( FileOneBit,  "\n" );		// end the line
	fprintf( FileTwoBit,  "\n" );		// end the line
	fprintf( FileFourBit, "\n" );		// end the line
	fprintf( FileSummary, "%.4f %.4f %.4f\n", SumInformationOneBit, SumInformationTwoBit, SumInformationFourBit );		// write the timestep on the beginning of the line
}
