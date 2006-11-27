#include <iostream.h>
#include <string>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector.h>
#include <list.h>
#include "alife_complexity_functions.h"

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

using namespace std;

int main()
{

	char* directory_name = 	"/Users/virgil/alife_complexity/olaf_fixedPatches15/brain/bestRecent/1000/";

	vector<string> Mnamesf = get_list_of_brainfunction_logfiles( directory_name );
	vector<string> Mnamesa =  get_list_of_brainanatomy_logfiles( directory_name );

	cout << "My Function List: " << endl;
	for(int i=0; i<Mnamesf.size(); i++)
	{
		cout << Mnamesf[i] << "\tLength: " << (Mnamesf[i]).length() << endl;
	}

	cout << "My Anatomy List: " << endl;
	for(int i=0; i<Mnamesa.size(); i++)
	{
		cout << Mnamesa[i] << "\tLength: " << (Mnamesa[i]).length() << endl;
	}

	gsl_matrix * activity = readin_brainfunction( (Mnamesf[0]).c_str() );


/* MATLAB CODE: 
        if (size(activity,2)>size(activity,1))
            activity = activity';
        end;
*/

	if( activity->size2 > activity->size1 )		//This will basically always be true.
	{
		gsl_matrix * temp = gsl_matrix_alloc( activity->size2, activity->size1);
		gsl_matrix_transpose_memcpy(temp, activity);
		gsl_matrix * temp2 = activity;
		activity = temp;		// activity = activity'
		gsl_matrix_free(temp2);
	}
	cout << "Size1: " << activity->size1 << "   Size2: " << activity->size2 << endl;


/* MATLAB CODE:
	% add a tiny bit of noise to avoid NaN's in COR
	activity = activity+0.0001*randn(size(activity));

*/
	// Setup our random number generator for rand() ...
	const gsl_rng_type * T;
	gsl_rng * r;
	gsl_rng_env_setup();
	T = gsl_rng_mt19937;
	r = gsl_rng_alloc(T);
	time_t seed;
	time(&seed);
	gsl_rng_set(r, seed);


	int numrows = activity->size1;
	int numcols = activity->size2;


	for(int i=0; i<numrows; i++)
	{
		for(int j=0; j<numcols; j++)
		{
			gsl_matrix_set(activity, i, j, gsl_matrix_get(activity, i,j) + 0.0001*gsl_ran_ugaussian(r));
		}
	}


/* MATLAB CODE:
        % induce Gaussian pdf
        o = gsamp( activity(size(activity,1)-min(500,size(activity,1))+1:size(activity,1),:)');
        o = o';

        % length of time used for "early" vs. "late" (birth/death) comparisons
        deltat = min(200,size(activity,1));
        o_birth = o(1:deltat,:);
        o_death = o(end-deltat+1:end,:);
        activity_birth = activity(1:deltat,:);			//this code makes no sense. They will be the same as activity
        activity_death = activity(end-deltat+1:end,:);		//this code makes no sense. They will be the same as activity
*/

	// AT THIS TIME WE DO NOT COMPUTE THE GSAMP().
	gsl_matrix * o = activity;

	int deltat;
	if( activity->size1 < 200 )
		deltat = activity->size1;
	else
		deltat = 200;

	gsl_matrix_view MV_activity_birth = gsl_matrix_submatrix( activity, 0, 0, deltat, activity->size2 );	
	gsl_matrix_view MV_activity_death = gsl_matrix_submatrix( activity, activity->size1 - deltat, 0, deltat, activity->size2 );	

	gsl_matrix * activity_birth = &(MV_activity_birth.matrix);
	gsl_matrix * activity_death = &(MV_activity_death.matrix);

	gsl_matrix * o_birth = activity_birth;		//This can be replaced with gsamp()'ing
	gsl_matrix * o_death = activity_death;		//This can be replaced with gsamp()'ing

/* MATLAB CODE:
        % sum of activity (always use raw numbers)
        sa = sum(activity);
        sa_birth = sum(activity_birth);
        sa_death = sum(activity_death);
        
        % get covariance and correlation
        COV = cov(o); COR = COV./(sqrt(diag(COV))*sqrt(diag(COV))');
        COVb = cov(o_birth); CORb = COVb./(sqrt(diag(COVb))*sqrt(diag(COVb))');
        COVd = cov(o_death); CORd = COVd./(sqrt(diag(COVd))*sqrt(diag(COVd))');
*/
	// We calculated sa previously in the gaussian noise loop.

	double sa = 0;
	int rows=activity->size1;
	int cols=activity->size2;

        for(int i=0; i<rows; i++)
        {
                for(int j=0; j<cols; j++)
                {
                        sa += gsl_matrix_get(activity, i, j);
                }
        }

	double sa_birth=0;
	rows=activity_birth->size1;
	cols=activity_birth->size2;

        for(int i=0; i<rows; i++)
        {
                for(int j=0; j<cols; j++)
                {
                        sa_birth += gsl_matrix_get(activity_birth, i, j);
                }
        }

	double sa_death=0;
	rows=activity_death->size1;
	cols=activity_death->size2;

        for(int i=0; i<rows; i++)
        {
                for(int j=0; j<cols; j++)
                {
                        sa_death += gsl_matrix_get(activity_death, i, j);
                }
        }
	
// NOTE: Even after extensive testing and debugging, some of the values in COR seem to deviate from the MATLAB code.
// After all this testing and finding nothing wrong in the C++, I think this is just due to the higher decimal 
// precision of the GSL matrices.

	gsl_matrix * COV = mCOV( o );
	gsl_matrix * COR = COVtoCOR( COV );

	gsl_matrix * COVb = mCOV( o_birth );
	gsl_matrix * CORb = COVtoCOR( COVb );

	gsl_matrix * COVd = mCOV( o_death );
	gsl_matrix * CORd = COVtoCOR( COVd );


/* MATLAB CODE:
        % get complexity and integration
        Cplx(l,k) = calcC_det3(COR); Cplxb(l,k) = calcC_det3(CORb); Cplxd(l,k) = calcC_det3(CORd);
        Intg(l,k) = calcI_det2(COR); Intgb(l,k) = calcI_det2(CORb); Intgd(l,k) = calcI_det2(CORd);
        
        % null out creatures with short lives
        %if (size(activity,1)<500) Cplx(l,k) = 0; Intg(l,k) = 0; end;
*/

	cout << "Activity\tCplx: " << calcC_det3(COR) << "\t\tIntg: " << calcI_det2(COR) << endl;
	cout << "Born\t\tCplx: " << calcC_det3(CORb) << "\t\tIntg: " << calcI_det2(CORb) << endl;
	cout << "Death\t\tCplx: " << calcC_det3(CORd) << "\t\tIntg: " << calcI_det2(CORd) << endl;




/* MATLAB CODE:
        % get MI between inputs and processing units
        fname = Mnamesa((l-1)*3+2,:);                           %VVV: Here we set the fname to deathAnatomy File.

        % strip fnames of spaces at the end
        fname = fname(fname~=' ');       
        eval(['load /home/osporns/larry/',runnumber,'/',num2str(dirnumber),'/',fname]);
*/
	cout << "Getting cij matrix from " << Mnamesa[1].c_str() << endl;
	gsl_matrix * cij = readin_brainanatomy( (Mnamesa[1]).c_str() );


/* MATLAB CODE:
        % convert cij to binary
        CIJ = (cij~=0);
        CIJ = double(CIJ>0);
        
        % throw away for now the bias neuron
        CIJ = CIJ(1:size(CIJ,1)-1,1:size(CIJ,2)-1);

        % invert rows and columns to make consistent with OLS work
        CIJ = CIJ';
*/

	gsl_matrix * CIJ = gsl_matrix_alloc(cij->size1-1, cij->size2-1);		// is minus 1 because we toss out the bias neuron

	int side_length = cij->size1;		//matrix cij is square so we only need 1 side.

	// We'll use this later.

	for(int i=0; i < side_length-1; i++)
	{
		for(int j=0; j < side_length-1; j++)
		{
			if( gsl_matrix_get(cij, i, j) != 0 )
				gsl_matrix_set(CIJ, j, i, 1);		// Here we swap the i's and j's so we transpose the matrix for free
			else
				gsl_matrix_set(CIJ, j, i, 0);		// Here we swap the i's nad j's so we transpose the matrix for free
		}
	}

	print_matrix_row(CIJ, 27 );

/* MATLAB CODE:
        % how many neurons total?
        Neu(l,k) = size(CIJ,1);

        % how many input neurons?
        Inp(l,k) = size(CIJ,1) - nnz(sum(CIJ));		% VVV:  The sum() sums down the columns
        Inp_id = find(sum(CIJ)==0);

        % how many other neurons (internal and output = processing)?
        Pro(l,k) = Neu(l,k)-Inp(l,k);
        Pro_id = setdiff(1:Neu(l,k),Inp_id);
*/

	int Neu = CIJ->size1;
	side_length = CIJ->size1;
	
	vector<int> Inp_id;
	vector<int> Pro_id;
	for(int i=0; i< side_length; i++)
	{
		bool flag=false;
		for(int j=0; j< side_length; j++)
		{
			if( gsl_matrix_get(CIJ, i, j) )
			{
				flag=true;
				Pro_id.push_back( i );
				break;
			}
		}

		if( flag == false )
		{
			Inp_id.push_back( i );
		}
	}

	int Inp = Inp_id.size();
	int Pro = Pro_id.size();

	cout << "Inp_id: " << Inp << endl;
	for(int i=0; i< Inp_id.size(); i++ )
	{
		cout << Inp_id[i] << " ";
	}
	cout << endl;

	cout << "Pro_id: " << Pro << endl;
	for(int i=0; i< Pro_id.size(); i++ )
	{
		cout << Pro_id[i] << " ";
	}
	cout << endl;


/* MATLAB CODE:
        % mutual information between input and processing units (under
        % Gaussian assumptions)
        pie1 = 2*pi*exp(1);
        MI_InpPro(l,k) = (log(det(COV(Inp_id,Inp_id))*pie1^Inp(l,k))/2 + log(det(COV(Pro_id,Pro_id))*pie1^Pro(l,k))/2 - ...
            log(det(COV)*pie1^Neu(l,k))/2);
*/

	// This looks like the gsamp()'ing done earlier.  So am this until hearing back from Olaf.

	
/* MATLAB CODE:
        % get complexity of input versus processing units
        Cplx_Inp(l,k) = calcC_det3(COR(Inp_id,Inp_id));
        Cplx_Pro(l,k) = calcC_det3(COR(Pro_id,Pro_id));
        Cplx_Inp_birth(l,k) = calcC_det3(CORb(Inp_id,Inp_id));
        Cplx_Pro_birth(l,k) = calcC_det3(CORb(Pro_id,Pro_id));
        Cplx_Inp_death(l,k) = calcC_det3(CORd(Inp_id,Inp_id));
        Cplx_Pro_death(l,k) = calcC_det3(CORd(Pro_id,Pro_id));
*/

	// Have left off at this point because we've gotten everything (Complexity, Integration, Parsing of Log Files), that we need for this pass.
	// -Virgil
	// 2/27/2006


/* MATLAB CODE: 
        % get complexity of input versus processing units
        Cplx_Inp(l,k) = calcC_det3(COR(Inp_id,Inp_id));
        Cplx_Pro(l,k) = calcC_det3(COR(Pro_id,Pro_id));
        Cplx_Inp_birth(l,k) = calcC_det3(CORb(Inp_id,Inp_id));
        Cplx_Pro_birth(l,k) = calcC_det3(CORb(Pro_id,Pro_id));
        Cplx_Inp_death(l,k) = calcC_det3(CORd(Inp_id,Inp_id));
        Cplx_Pro_death(l,k) = calcC_det3(CORd(Pro_id,Pro_id));
            
        % get PHI for processing units
        %[max_MIB, max_MIB_cluster, max_MIB_bipcluster] = quick_PHI(cij(Pro_id,Pro_id), 0.00001, 1, 0.5, 1);
        %maxMIB(l) = max_MIB;
        
        % sum of overall activation
        sum_ac(l,k) = sum(sa);
        sum_acI(l,k) = sum(sa(Inp_id));
        sum_acP(l,k) = sum(sa(Pro_id));
        sum_ac_birth(l,k) = sum(sa_birth);
        sum_acI_birth(l,k) = sum(sa_birth(Inp_id));
        sum_acP_birth(l,k) = sum(sa_birth(Pro_id));
        sum_ac_death(l,k) = sum(sa_death);
        sum_acI_death(l,k) = sum(sa_death(Inp_id));
        sum_acP_death(l,k) = sum(sa_death(Pro_id));
        sum_ac_perunit(l,k) = sum(sa)/length(sa);
        sum_acI_perunit(l,k) = sum(sa(Inp_id))/length(Inp_id);
        sum_acP_perunit(l,k) = sum(sa(Pro_id))/length(Pro_id);
        
        % discretize
        % nst = 16 tends to be max allowable (16*16*3 = 768)
        nst = 16;
        Mstates = smd_discrete(activity,nst);
        % kludge
        Mstates(Mstates==0) = 1;
        
        % calculate entropy
        [H,B_star,H_inf,Sigma_H] = smd_H(Mstates,nst);
        mH_Inp(l,k) = mean(nonzeros(H(Inp_id)));
        mH_Pro(l,k) = mean(nonzeros(H(Pro_id)));
        mH_all(l,k) = mean(nonzeros(H));
        
        % MI, using Roulston
        units = 1:Neu(l,k); t = 0;
        [MIt_inf,MIt_obs]=smd_MIt(Mstates,units,nst,t);
        mMI_Pro(l,k) = mean(nonzeros(triu(MIt_inf(Pro_id,Pro_id),1)));
        mMI_Inp(l,k) = mean(nonzeros(triu(MIt_inf(Inp_id,Inp_id),1)));
        mMI_IP(l,k) = mean(nonzeros(MIt_inf(Inp_id,Pro_id)));
        maxMI_Inp(l,k) = max(nonzeros(triu(MIt_inf(Inp_id,Inp_id),1)));
        
        %for i=1:Neu(l,k)
        %    for j=1:Neu(l,k)
        %        [t_i2j(i,j), t_j2i(i,j)] = csl_schreiber(activity(:,i),activity(:,j),8);
        %    end;
        %end;
        %mTE_IPi2j(l,k) = mean(nonzeros(t_i2j(Inp_id,Pro_id)));
        %mTE_IPj2i(l,k) = mean(nonzeros(t_j2i(Inp_id,Pro_id)));
        
        % a few other useful things
        N(l,k) = numneu;
        F(l,k) = filnum;
        fit(l,k) = fitness;
        life(l,k) = size(activity,1);

        %disp([num2str(N(l)),' ',num2str(Cplx(l)),' ',num2str(Intg(l))]);
        %Cplx

    end; % VVV: This just ends a loop we don't use anyway (Looped through all BrainFunction Filenames)

%    eval(['save /home/osporns/larry/',runnumber,'/',num2str(dirnumber) ...
%        ,'/probe_activity_data.mat N F Cplx Intg fit Neu Inp Pro MI_InpPro Cplx_Inp Cplx_Pro']);

    % summary data
    mCplx(k) = mean(Cplx(:,k));
    sCplx(k) = std(Cplx(:,k));
    
    mCplxI(k) = mean(Cplx_Inp(:,k));
    sCplxI(k) = std(Cplx_Inp(:,k));

    mCplxP(k) = mean(Cplx_Pro(:,k));
    sCplxP(k) = std(Cplx_Pro(:,k));

    mIntg(k) = mean(Intg(:,k));
    sIntg(k) = std(Intg(:,k));

    mfit(k) = mean(fit(:,k));
    sfit(k) = std(fit(:,k));
    
    mMIIP(k) = mean(MI_InpPro(:,k));
    sMIIP(k) = std(MI_InpPro(:,k));
    
    mNeu(k) = mean(Neu(:,k));
    sNeu(k) = std(Neu(:,k));
    
    mInp(k) = mean(Inp(:,k));
    sInp(k) = std(Inp(:,k));

    mPro(k) = mean(Pro(:,k));
    sPro(k) = std(Pro(:,k));

end;		% VVV: This just ends a loop we don't use anyway. (Looped through all timesteps)
*/






	return 0;
}

