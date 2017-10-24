#include "ros/ros.h"
#include "ros/time.h"
#include "arduino_msgs/StampedInt32.h"
#include "arduino_msgs/StampedInt64.h"
#include "arduino_msgs/StampedFloat32.h"
#include "arduino_msgs/StampedFloat64.h"
#include <sstream>
#include <math.h>
#include <vector>

using namespace std;


#define X 0
#define Y 1
#define PI 3.14159265f
#define CORRECAO_GIRO 9
int STATE;
#define ANDAR 1

	ros::Publisher pubM_int64;
	ros::Publisher pubM_float64;
	ros::Publisher pubN_int32;
	ros::Publisher pubN_float32;
	ros::Publisher pubVis_int32;

	ros::Subscriber subM_int64;
	ros::Subscriber subM_float64;
	ros::Subscriber subN_int32;
	ros::Subscriber subN_float32;
	ros::Subscriber subVis_float64;

	double cow_pos_x1, cow_pos_x2, cow_pos_z1, cow_pos_z2, cow_pos_err;
	
/*---------------------------------------definicoes ROS-------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/

/*-------------------------------------definicoes locomocao---------------------------------------*/

	#define GIRA 300
	#define VEL_REF_DIR 301
	#define VEL_REF_ESQ 302
	#define TRAVAR 303

	#define ACABOU_GIRO 1
	#define ACABOU_ATUAL 2
	#define DISTANCIA_MIN_AJUSTE_VACA 30.0f
/*------------------------------------------------------------------------------------------------*/

/*-----------------------------------declaracoes das funcoes--------------------------------------*/
	bool ehSonar(int id);
	bool ehToque(int id);
	void messageMInt64Cb( const arduino_msgs::StampedInt64& aM_int64_msg);
	void messageMFloat64Cb( const arduino_msgs::StampedFloat64& aM_float64_msg);
	void messageNInt32Cb( const arduino_msgs::StampedInt32& aN_int32_msg);
	void messageNFloat32Cb( const arduino_msgs::StampedFloat32& aN_float32_msg);
	void messageVisFloat64Cb( const arduino_msgs::StampedFloat64& vis_float64_msg);
	void initROS();
	void SendFloatMega(int id, double data);
	void SendIntMega(int id, long long int data);	
	void SendFloatUno(int id, float data);
	void SendIntUno(int id, long int data);
	void SendVel(float DIR, float ESQ);
	void GiraEmGraus(float angulo);
	float DeGrausParaRadianos(float angulo);
	float DeRadianosParaDegraus(float angulo);
	vector<float> AnaliseLugarDaVaca(float x1, float y1, float x2, float y2, float theta);
	void TrajetoriaSuaveAteAVaca(float x1, float y1, float x2, float y2, float theta);
	void andaRetoDistVel(float dist, float vel);
	void algoritmo();
/*------------------------------------------------------------------------------------------------*/

/*---------------------------------definicoes dos sensores US-------------------------------------*/

	#define US1 0
	#define US2 1
	#define US3 2
	#define US4 3
	#define US5 4
	#define US6 5
	#define US7 6
	#define US8 7
	#define US_MAX_DIST 200
	#define NUM_IDEN_US 100
	#define QUANTIDADE_SENSOR_US 8
	#define TAMANHO_MEDIANA 5
	#define VALOR_MEDIANA 2
	//sinal chega do arduino com o id(USn) = (n-1) + NUM_IDEN_US... exemplo: id do US5 = (5-1) + 100 = 104

	#define alfa_US 0.8f

	class Us{
  		public:
	  		float valor;
	  		vector<float> valores;
	  		long long int vezes_lido;
		Us(){
			valores = vector<float>(TAMANHO_MEDIANA);
		}
	};

	vector<Us> ultrassom(QUANTIDADE_SENSOR_US);
/*------------------------------------------------------------------------------------------------*/

/*-----------------------------definicoes dos sensores de TOQUE-----------------------------------*/

	#define TOQUE1 0
	#define TOQUE2 1
	#define TOQUE3 2
	#define TOQUE4 3
	#define TOQUE5 4
	#define TOQUE6 5
	#define TOQUE7 6

	#define TOQUE8 7
	#define TOQUE9 8

	#define NUM_IDEN_TOQUE 200
	#define QUANTIDADE_SENSOR_TOQUE 9

	vector<bool> toque(QUANTIDADE_SENSOR_TOQUE);//(QUANTIDADE_SENSOR_TOQUE, false);
/*------------------------------------------------------------------------------------------------*/

#define QUANTIDADE_MOTORES_GARRA  5
#define NUM_IDEN_VISION 500

class Ocupacao{
	public:
		bool giro;
		vector<bool> motoresGarra;
		float tempoAn;
		bool andando;
	Ocupacao(){
		motoresGarra = vector<bool>(QUANTIDADE_MOTORES_GARRA);
	}	

};

Ocupacao ocupado;

template<typename ItemType>
unsigned Partition(ItemType* array, unsigned f, unsigned l, ItemType pivot)
{
    unsigned i = f-1, j = l+1;
    while(true)
    {
        while(pivot < array[--j]);
        while(array[++i] < pivot);
        if(i<j)
        {
            ItemType tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
        }
        else
            return j;
    }
}

template<typename ItemType>
void QuickSortImpl(ItemType* array, unsigned f, unsigned l)
{
    while(f < l)
    {
        unsigned m = Partition(array, f, l, array[f]);
        QuickSortImpl(array, f, m);
        f = m+1;
    }
}

template<typename ItemType>
void QuickSort(ItemType* array, unsigned size)
{
    QuickSortImpl(array, 0, size-1);
}

bool ehSonar(int id)
{
	id = id - NUM_IDEN_US;
	if (id < QUANTIDADE_SENSOR_US && id >= 0)	 return true;
	else	return false;
}

bool ehToque(int id)
{
	id = id - NUM_IDEN_TOQUE;
	if (id < QUANTIDADE_SENSOR_TOQUE && id >= 0)	 return true;
	else	return false;
}

void messageMInt64Cb( const arduino_msgs::StampedInt64& aM_int64_msg)
{
	if(ehSonar(aM_int64_msg.id))
	{
		int usPos = aM_int64_msg.id - NUM_IDEN_US; 
		int valor ;
		valor = (aM_int64_msg.data <= 0.1) ? US_MAX_DIST : aM_int64_msg.data;	
		//sonar[aN_int32_msg.id - NUM_IDEN_US] = valor * alfa_US + sonar[aN_int32_msg.id - NUM_IDEN_US] * (1 - alfa_US);
		float valorf = (float)valor ; 
		long long int iterator = ultrassom[usPos].vezes_lido ;
	    ultrassom[usPos].valores[iterator % TAMANHO_MEDIANA] = valorf;
	    float valores[TAMANHO_MEDIANA];
	    for(unsigned i = 0; i < TAMANHO_MEDIANA; ++i)
	      	valores[i] = ultrassom[usPos].valores[i];
	    QuickSort(valores,TAMANHO_MEDIANA);
	    //ultrassom[usPos].value = values[MEDIAN_VALUE];
	    valorf = valores[VALOR_MEDIANA];
	    ultrassom[usPos].valor = valorf * alfa_US + ultrassom[usPos].valor * ( 1- alfa_US);
	    ultrassom[usPos].vezes_lido++;
	}else if (ehToque(aM_int64_msg.id)) 
		toque[aM_int64_msg.id - NUM_IDEN_TOQUE] = aM_int64_msg.data;
}


void messageMFloat64Cb( const arduino_msgs::StampedFloat64& aM_float64_msg)
{
	
}

void messageNInt32Cb( const arduino_msgs::StampedInt32& aN_int32_msg)
{
	if (aN_int32_msg.id == ACABOU_GIRO)		ocupado.giro = false;
}

void messageNFloat32Cb( const arduino_msgs::StampedFloat32& aN_float32_msg)
{

}

void messageVisFloat64Cb( const arduino_msgs::StampedFloat64& vis_float64_msg)
{
	if(vis_float64_msg.id == NUM_IDEN_VISION + 1)
	{
		//cout << "Got x1: ";
		cow_pos_x1 = vis_float64_msg.data;
		//cout << vis_float64_msg.data;
		//cout << "\n";
	} else if(vis_float64_msg.id == NUM_IDEN_VISION + 2) {
		//cout << "Got z1: ";
		cow_pos_z1 = vis_float64_msg.data;
		//cout << vis_float64_msg.data;
		//cout << "\n";
	} else if(vis_float64_msg.id == NUM_IDEN_VISION + 3) {
		//cout << "Got x2: ";
		cow_pos_x2 = vis_float64_msg.data;
		//cout << vis_float64_msg.data;
		//cout << "\n";
	} else if(vis_float64_msg.id == NUM_IDEN_VISION + 4) {
		//cout << "Got z2: ";
		cow_pos_z2 = vis_float64_msg.data;
		//cout << vis_float64_msg.data;
		//cout << "\n";
	} else if(vis_float64_msg.id == NUM_IDEN_VISION + 5) {
		//cout << "Got error value: ";
		cow_pos_err = vis_float64_msg.data;
		//cout << vis_float64_msg.data;
		//cout << "\n";
	} else {
		cout << "Got something weird\n";
	}
}

void Delay(double time)
{
    double t1=0, t0=0;
    t0 = ros::Time::now().toSec();
    while((t1-t0)<time){
        t1 = ros::Time::now().toSec();
        ros::spinOnce();
    }
}

void initROS(ros::NodeHandle nh)
{
	pubM_int64 = nh.advertise<arduino_msgs::StampedInt64>("raspberryM_int64", 1000);
	pubM_float64 = nh.advertise<arduino_msgs::StampedFloat64>("raspberryM_float64", 1000);
	subM_int64 = nh.subscribe("arduinoM_int64", 1000, messageMInt64Cb);
	subM_float64 = nh.subscribe("arduinoM_float64", 1000, messageMFloat64Cb);

	pubN_int32 = nh.advertise<arduino_msgs::StampedInt32>("raspberryN_int32", 1000);
	pubN_float32 = nh.advertise<arduino_msgs::StampedFloat32>("raspberryN_float32", 1000);
	subN_int32 = nh.subscribe("arduinoN_int32", 1000, messageNInt32Cb);
	subN_float32 = nh.subscribe("arduinoN_float32", 1000, messageNFloat32Cb);

	pubVis_int32 = nh.advertise<arduino_msgs::StampedInt32>("Vision_int32", 1000); 
	subVis_float64 = nh.subscribe("Vision_float64", 1000, messageVisFloat64Cb);

	ocupado.giro = false;
	fill(toque.begin(), toque.end(), false);
	fill(ocupado.motoresGarra.begin(), ocupado.motoresGarra.end(), false);
	ocupado.andando = false; 
}

void SendFloatMega(int id, double data)
{   
	arduino_msgs::StampedFloat64 float64_msg;
	float64_msg.id = id;
	float64_msg.data = data;
	pubM_float64.publish(float64_msg);
}


void SendIntMega(int id, long long int data)
{    
	arduino_msgs::StampedInt64 int64_msg;
	int64_msg.id = id;
	int64_msg.data = data;
	pubM_int64.publish(int64_msg);
}

void SendFloatUno(int id, float data)
{    
	arduino_msgs::StampedFloat32 float32_msg;
	float32_msg.id = id;
	float32_msg.data = data;
	pubN_float32.publish(float32_msg);
}

void SendIntUno(int id, long int data)
{    
	arduino_msgs::StampedInt32 int32_msg;
	int32_msg.id = id;
	int32_msg.data = data;
	pubN_int32.publish(int32_msg);
}

void SendIntVision(int id, long int data)
{    
	arduino_msgs::StampedInt32 int32_msg;
	int32_msg.id = id;
	int32_msg.data = data;
	pubVis_int32.publish(int32_msg);
}

void SendVel(float DIR, float ESQ)
{
	SendFloatUno(VEL_REF_DIR, DIR);
	SendFloatUno(VEL_REF_ESQ, ESQ);
}

void GiraEmGraus(float angulo)
{
	ocupado.giro = true;
	SendFloatUno(GIRA, angulo - (float(CORRECAO_GIRO)));
	while(ocupado.giro)
	{
		ros::spinOnce();
	}
}

float DeGrausParaRadianos(float angulo)
{
	return (angulo*PI)/180;
}

float DeRadianosParaDegraus(float angulo)
{
	return (angulo*180)/PI;
}

vector<float> AnaliseLugarDaVaca(float x1, float y1, float x2, float y2, float theta)
{
	float  distancia_direta, centro_da_entrada[2];
	vector<float> ponto_do_viro(2);
	centro_da_entrada[X] = ((x2-x1)/2);
	centro_da_entrada[Y] = ((y2-y1)/2);
	distancia_direta = sqrt(pow(centro_da_entrada[X],2) + pow(centro_da_entrada[Y],2));
	if (distancia_direta<DISTANCIA_MIN_AJUSTE_VACA)
		///se ajeita (dar re e ganhar espaco para chegar melhor na vaca) e faz de novo o processo
	ponto_do_viro[X] = centro_da_entrada[X] - (DISTANCIA_MIN_AJUSTE_VACA*cos(DeGrausParaRadianos(90-theta)));
	ponto_do_viro[Y] = centro_da_entrada[Y] - (DISTANCIA_MIN_AJUSTE_VACA*sin(DeGrausParaRadianos(90-theta)));
	return ponto_do_viro;
}

void TrajetoriaSuaveAteAVaca(float x1, float y1, float x2, float y2, float theta)
{
	float alfa, distancia_ate_ponto_do_giro;
	vector<float> v = AnaliseLugarDaVaca(x1, y1, x2, y2, theta);
	alfa = DeRadianosParaDegraus(atan2(v[X],v[Y]));
	//gira(alfa)
	distancia_ate_ponto_do_giro = sqrt(pow(v[X],2) + pow(v[Y],2));
	//anda(distancia_ate_ponto_do_giro)
	//gira(-(alfa+theta))
	//anda(DISTANCIA_MIN_AJUSTE_VACA)
}

void andaRetoDistVel(float dist, float vel)
{
	double tempo=0;
	tempo = dist*2.682871209/abs(vel); /// de rotacoes por segundo para metros por segundo
	SendVel(vel, vel);
	Delay(tempo);
	SendVel(0.0,0.0);
	SendFloatUno(TRAVAR,0);
}

void algoritmo()
{
	//Delay(2);
	//andaRetoDistVel(1,1);
	Delay(2);
	//andaRetoDistVel(1.1,1);
	GiraEmGraus(180.0);
	//andaRetoDistVel(1.1,1);
	Delay(15);
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "first_test");
	ros::NodeHandle nh;
	initROS(nh);

	ros::Rate loop_rate(10);
	while (ros::ok())
	{
		SendIntVision(500, 0);
		cout << "Calling vision algorithm\n";
		Delay(2);
		//algoritmo();	
		ros::spinOnce();
		loop_rate.sleep();
		float xmed = (cow_pos_x1 + cow_pos_x2)/2;
		float zmed = (cow_pos_z1 + cow_pos_z2)/2;
		float mod = sqrt(pow(xmed, 2) + pow(zmed, 2));
		float ang = 180*atan2(zmed, xmed)/3.1416;
		cout << "xmed = ";
		cout << xmed;
		cout << "\n";
		cout << "zmed = ";
		cout << zmed;
		cout << "\n";
		cout << "mod = ";
		cout << mod;
		cout << "\n";
		cout << "ang = ";
		cout << ang;
		cout << "\n";
	}

	return 0;
}