#ifndef SFDFORTRAN
#define SFDFORTRAN

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>
#include <QString>
#include <QStringList>

#define FILENAME_SFD_VAR_BD      "cfgsfd/sfd_var_guide.txt"
#define FILENAME_SFD_BLOCK_MODEL "cfgsfd/d009/b"
#define FILENAME_SFD_MDI         "cfgsfd/d009/mdi"
#define FILENAME_SFD_MDA         "cfgsfd/d009/mda"

// define для файла блочной модели 'b'
#define POSITION_NUM_BLOCK_TYPE_ID      4
#define POSITION_NUM_BLOCK_ID_2SYMBOL   7
#define POSITION_BLOCK_EQ_QNTY          10
#define POSITION_BLOCK_ORDINAL_NUM       0
#define POSITION_BLOCK_NAME             15

#define POSITION_VAR_ORDINAL_NUM        12
#define POSITION_VAR_UNKNOWN_SIGN        8
#define POSITION_VAR_PRINT_SIGN          3

#define POSITION_BLOCK_VARIABLE_INITVAL 14
#define POSITION_BLOCK_VARIABLE_NAME    15+17

// define для файла sfd_var_bd
#define POSITION_VARGUIDE_ONUM          10
#define POSITION_VARGUIDE_TYPE          31
#define POSITION_VARGUIDE_NAME          35
#define POSITION_VARGUIDE_DESCRIPTION   57

// define для файла mdi
#define POSITION_MDI_INIT_VALUE          14
#define POSITION_MDI_VAR_NAME            32


// тип строки
#define STRING_VARIABLE   1
#define STRING_EQUATION   2

#define  MAX_SFD_CASE_ID 110


#define  SUBST_SOURCE_STAND_ALONE_DATA            1
#define  SUBST_SOURCE_TECHNICAL_CONDITIONS        2
#define  SUBST_SOURCE_UNKNOWN_VARIABLE            3
#define  SUBST_SOURCE_CALC_UNKNOWN_VARIABLE       4
#define  SUBST_SOURCE_BOUNDARY_OUT_OF_EQUATIONS   5
#define  SUBST_SOURCE_MEASURED_VARIABLE           6
#define  SUBST_SOURCE_CLOSED_AND_MEASURED         7
#define  SUBST_SOURCE_CONSTANT                    8
#define  SUBST_SOURCE_EXPERT_AMENDMENTS           9
#define  SUBST_SOURCE_SELECT_CONSTANT            10

#define  SUBST_SOURCE_CONTROL_VARIABLE           11
#define  SUBST_SOURCE_SFD_EQUATIONS              12

#define  CHTYPE_POLINOM                   "polinom"
#define  CHTYPE_NUMBER                    "number"
#define  CHTYPETABLE                      "table"

#define  JSON_STAND_ALONE_DATA             "StandAloneData"
#define  JSON_TECHNICAL_CONDITIONS         "TechnicalConditions"
#define  JSON_UNKNOWN_VARIABLE             "UnknownVariable"
#define  JSON_CALC_UNKNOWN_VARIABLE        "CalcUnknownVariable"
#define  JSON_BOUNDARY_OUT_OF_EQUATIONS    "BoundaryOutOfEquations"
#define  JSON_MEASURED_VARIABLE            "MeasuredVariable"
#define  JSON_CONTROL_VARIABLE             "ControlVariable"
#define  JSON_CONSTANT                     "ConstantLibrary"
#define  JSON_CLOSED_AND_MEASURED_VARIABLE "ClosedAndMeasuredVariable"
#define  JSON_SELECT_CONSTANT              "SelectConstant"
#define  JSON_EXPERT_AMENDMENTS            "ExpertAmendments"
#define  JSON_SFD_EQUATIONS                "Equations"

#define  jSfdObject                       "SfdObject"
#define  jSfdObjectKeyId                  "IdObject"
#define  jSfdObjectKeyType                "TypeObject"
#define  jSfdObjectKeyDecription          "Decription"
#define  jSfdObjectKeyValue               "Value"
#define  jSfdObjectLink                   "ObjectLink"

#define  jFactoryNum                      "FactoryNum"
#define  jNameUnit                        "NameUnit"


/*---------------------------------------------------------------------------------------------*/
// для составления справочника guide, согласно файлу sfd_var_guide.txt
/*---------------------------------------------------------------------------------------------*/
typedef struct {


  QString OderNum;
  QString Name;
  QString sInitValue; // в guide - не нужное поле
  QString Description;
  QString TypeVar;
// TypeVar:
//  1- Переменная рассчитывается вне системы уравнений
//  2- Переменная рассчитывается как неизвестная в системе уравнений
// 21- Переменная рассчитывается как неизвестная в системе уравнений и измеряется
//  3- Константа из ТУ
// 31- Константа из автономных испытаний
//  4- Измеренные замыкающие парамеры системы уравнений

  // преобразованные и новые свойства
  bool    ConvFlg;
  double  InitValue; // в guide - не нужное поле
  int     Oder;
  int     TypeV;

  QString ID;


} TVariableGuide;


/*---------------------------------------------------------------------------------------------*/
// для описания свойства переменой после парсинга блочной модели
/*---------------------------------------------------------------------------------------------*/
typedef struct {

  QString Name;
  QString sInitValue;  // первое значение из блочной модели
  QString UnknownSign; // неизвестная в системе уравнений !!! уточнить у всех ли переменных в бл.модели флаг должен быть одинаков
  QString PrintSign;   // флаг печати в выходной файл

  // преобразованные и новые свойства
  QString SfdObjectID;  // сгенерированный идентификатор в зависимости от многих факторов
  QString ObjectType;   // тип объекта: число или полином (пока два варианта)

  QString Description;  // фактичческое описание переменной в Variable Guide

  bool    GuideFlg;    // в guide переменная присутствует
  int     TypeV;       // копия типа из guide
  QString ConvertName; // сконвертироывнное имя переменной

  bool    UnknownVarFlg;
  bool    PrintFlg;

  /*bool    ConvFlg;
  double  InitValue;
  int     OrdinalNum;
  int     TypeV;*/

  QStringList PolinomVars;    // полиномные переменные из блочной модели
  QStringList PolinomValue;   // значения для образуемых характеристик


  bool    DeleteFlg;     // признак удаления переменной (если она составная часть характеристики)
  int     Substitution;  // признак замещения в новой конфиге. (если не ноль, то Substitution = "код источника") (у переменной есть внешний источник/справочник)
                         // 1-справочник ТУ
                         // 2-данные автономных испытаний
} TRenewVariable;

/*---------------------------------------------------------------------------------------------*/
// для описания инкапусулированя свойств переменой при парсинге блочной модели
/*---------------------------------------------------------------------------------------------*/
typedef struct {

  QString Name;
  QString sInitValue;
  QString OrdinalN;    // порядковый номер в блоке
  QString UnknownSign; // неизвестная в системе уравнений
  QString PrintSign;   // флаг печати в выходной файл

  // преобразованные и новые свойства
  bool    ConvFlg;
  double  InitValue;
  QString ID;
  QString Description; //описание переменной в Variable Guide
  int     OrdinalNum;
  bool    UnknownVarFlg;
  bool    PrintFlg;
  int     TypeV;
  int     Substitution;  // признак замещения в новой конфиге. (если не ноль, то Substitution = "код источника") (у переменной есть внешний источник/справочник)
                         // 1-справочник ТУ
                         // 2-данные автономных испытаний
} TVariable;

/*---------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------*/
// для реконструкции блочной модели
/*---------------------------------------------------------------------------------------------*/
typedef struct {

  QString          Description;      // при FVectorVars == 0, текст в строке может считаться Description
                                     // при FVectorVars != 0, текст может считаться
  QString          PrimaryOrderNum;  // исходный порядковый номер в файле 'b'-блочной модели
                                     // этот номер как счетчик, сбоасывается в 1 когда в файле  'b' начтутся блоки с FVectorVars != 0
  QString          PrimaryEqQnty;    // Кол-во уравнений формируемых этим блоком. фактически является признаком наличия у блока переменных Fxvars
  QString          PrimaryCaseID;    // id-функции (номер id case() - по фортрану )
  QString          PrimaryInitValue; // начальное значение текстовое исходное
  QString          PrimaryBlockName; // имя без добаления "block"

  // преобразованные и новые свойства
//  bool   ConvFlg;
  QStringList      Equations;    // список уравнений, относящихся к вектору функций (FVectorVars)
  QStringList      NameVars;     // список имен переменных Variable
  QList<TVariable> Vars;         // переменные с их свойствами

  double           InitValue;    // преобразованное начальное значение (только для проверки преобразования)
  int              EqQnty;       // рассчитываемое в ходе формирования блока
  int              CaseID;       // преобразованный PrimaryCaseID из текста в int
  int              OldOrderNum;      //

//  QString Description;

} SFDBlock;

/*---------------------------------------------------------------------------------------------*/
// для описания правила, вычленения данных агрегатных испытаний
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  QString UnitCode;       // агрегат, которому принадлежит характеристика
  int     StrNum;         // номер строки в файле mda, где расположена исходная характеристика
  QString CharactId;      // код характеристики (в совокупности с UnitId образует уникальную ссылку)
  QString DescriptionVar; // переменная из файла sfd_var_guide, для взятия описания/обозначения характеристики
  QString Type;           // тип характеристики: полином или один коэффициент
  int     DegreePolynom;  // в списке MdiVar первый параметр указывает на порядок полинома (есть такие 2 характеристики) (признак)
  QStringList MdiVar;     // список переменных, согласно файлу mdi, образующих характеристику

  QJsonObject jOneUnitData; //результирующий JsonObject

} UnitConvertionRule;

/*---------------------------------------------------------------------------------------------*/
// для составления внутреннего справочника код->название по русски
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  QString UnitCode;      // Информационный код агрегата
  QString Name;          //
  QJsonObject jUnit;     // сюда свалим все объекты, относящиеся к unit
} TypeUnitList;

/*---------------------------------------------------------------------------------------------*/
// для группирования переменных, образующих единую полиноминальную характеристику
// эта информация прослеживается в sfd_var_bd.txt
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  QString FirstVar;       // первая переменная из файла sfd_var_bd, для взятия описания/обозначения характеристики
  int Count;              // кол-во строк, образующих характеристику
  QString CharactId;
  QStringList Vars;       // сюда занесем все имена переменных, относящиеся к характеристике
  QStringList Values;     // сюда занесем значения (из файла 'b')
} TVarsToCharacterRule;

/*---------------------------------------------------------------------------------------------*/
// для разбора данных файла 'mdi'
/*---------------------------------------------------------------------------------------------*/
typedef struct {

  QString sInitValue;
  QString VarName;

  // преобразованные и новые свойства
//  QString Description;
//  bool   ConvFlg;
//  double InitValue;
//  QString ID;

} EUnitInfo;

/*---------------------------------------------------------------------------------------------*/
// для замещения поля 'описание' некоторых уравнений в файле 'b' исходной блочной
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  QString  BlockName;
  int      CaseId;
  int      posnum;

  QString  NewEqTxt;

} TBlkModeQqSub;

/*---------------------------------------------------------------------------------------------*/
// для замещения некоторых переменных в файле 'b' исходной блочной
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  QString  VarName;
  QString  SubstVarName;

} TBlkModeVarSubst;

/*---------------------------------------------------------------------------------------------*/
// для обозначения блоков Case()
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  int      CaseId;
  QString  Desciption;
} TCaseDesciption;

/*---------------------------------------------------------------------------------------------*/
//
/*---------------------------------------------------------------------------------------------*/
typedef struct {
  int     GuideType;        // из var guide
//  int     SubstType;        // новый номер
  bool    ConvFlg;          // использовать для этого типа общее правило образования

  QString SectionId;    //
  QString SectionName;  // обозначение секции
} TTypeToSection;

#endif //SFDFORTRAN

