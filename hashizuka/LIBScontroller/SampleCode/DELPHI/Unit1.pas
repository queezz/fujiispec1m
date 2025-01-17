unit Unit1;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, DX2LIB, ExtCtrls;

type
  TForm1 = class(TForm)
    Button1: TButton;
    ScrollBar1: TScrollBar;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Label7: TLabel;
    ComboBox1: TComboBox;
    ComboBox2: TComboBox;
    ComboBox3: TComboBox;
    ComboBox4: TComboBox;
    Timer1: TTimer;
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure Button1Click(Sender: TObject);
    procedure ScrollBar1Change(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
    procedure ComboBox1Change(Sender: TObject);
    procedure ComboBox3Change(Sender: TObject);
    procedure FormDblClick(Sender: TObject);
  private
    { Private 宣言 }
    devid:  TDeviceID;
    function AllActive:boolean;

    function ReadMaxMinPos(dvid:TDeviceID; id:uint8):boolean;
    function WritePos (dvid:TDeviceID; id:uint8; pos:int32; var err:TErrorCode):boolean;
    function ReadPos (dvid:TDeviceID; id:uint8; var pos:int32; var err:TErrorCode) :boolean;

  public
    { Public 宣言 }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

// DLLのロードとポートオープン成功の状態取得
function TForm1.AllActive:boolean;
begin
  result := False;
  if DX2LIB_Active then
    if (devid <> 0) then result := DX2_Active (devid);
end;

// フォームを作成
procedure TForm1.FormCreate(Sender: TObject);
var
  i:integer;
  b:bool;
begin
  devid := 0;
  b:=DX2LIB_Load;    // DLLのダイナミックロード

  for i := 1 to 256 do
    ComboBox2.Items.Add('COM'+IntToStr(i));
  ComboBox2.ItemIndex := 0;

  for i := 0 to 253 do
    ComboBox4.Items.Add(IntToStr(i));
  ComboBox4.ItemIndex := 1;
end;

procedure TForm1.FormDblClick(Sender: TObject);
var
  v:PDXL_ModelInfo;
begin
try
try
  v:=DXL_GetModelInfo(devid, StrToInt(ComboBox4.Text));
except
end;
finally
end;
  writeln(
    'modelno=',v.modelno,#$D#$A,
    'name=',v.name,#$D#$A,
    'type=',ord(v.devtype),#$D#$A,
    'pos max=',v.positionlimit.max,#$D#$A,
    'pos min=',v.positionlimit.min,#$D#$A,
    'ang max=',v.anglelimit.max,#$D#$A,
    'ang min=',v.anglelimit.min,#$D#$A,
    'velo max=',v.velocitylimit.max,#$D#$A,
    'velo min=',v.velocitylimit.min,#$D#$A,
    'pwm max=',v.pwmlimit.max,#$D#$A,
    'pwm min=',v.pwmlimit.min,#$D#$A,
    'velo ratio=',v.velocityratio,#$D#$A,
    'cur ratio=',v.currentratio,#$D#$A,
    'pwm ratio=',v.pwmratio
  );
end;

// フォームを閉じる
procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  if AllActive then DX2_ClosePort(devid);  // DLLがロードされておりポートが開いていたら閉じる
  Timer1.Enabled := False;
  DX2LIB_Unload;  // DLLのアンロード
end;

// 指令可能な位置の範囲取得
function TForm1.ReadMaxMinPos(dvid:TDeviceID; id:uint8):boolean;
var
  err: TErrorCode;
  v16cw,v16ccw:uint16;
  v32cw,v32ccw:uint32;
  cw,ccw:int32;
begin
  result := false;
  if AllActive then
  begin
    case ComboBox1.ItemIndex of
      0:  //XL
      begin
        result := DX2_ReadWordData(dvid, id, 6, v16cw, err) and
          DX2_ReadWordData(dvid, id, 8, v16ccw, err);
        if result then
        begin
          cw := int16(v16cw);
          ccw := int32(v16ccw);
          if cw > ccw then
          begin
            ScrollBar1.Max := cw;
            ScrollBar1.Min := ccw;
          end else
          begin
            ScrollBar1.Max := ccw;
            ScrollBar1.Min := cw;
          end;
        end;
      end;
      1:  //X
      begin
        result := DX2_ReadLongData(dvid, id, 48, v32cw, err) and
          DX2_ReadLongData(dvid, id, 52, v32ccw, err);
        if result then
        begin
          cw := int32(v32cw);
          ccw := int32(v32ccw);
          if cw >= ccw then
          begin
            ScrollBar1.Max := cw;
            ScrollBar1.Min := ccw;
          end else
          begin
            ScrollBar1.Max := ccw;
            ScrollBar1.Min := cw;
          end;
        end;
      end;
      2:  //PRO
      begin
        result := DX2_ReadLongData(dvid, id, 36, v32cw, err) and
          DX2_ReadLongData(dvid, id, 40, v32ccw, err);
        if result then
        begin
          cw := int32(v32cw);
          ccw := int32(v32ccw);
          if cw > ccw then
          begin
            ScrollBar1.Max := cw;
            ScrollBar1.Min := ccw;
          end else
          begin
            ScrollBar1.Max := ccw;
            ScrollBar1.Min := cw;
          end;
        end;
      end;
      3:  //PRO+
      begin
        result := DX2_ReadLongData(dvid, id, 48, v32cw, err) and
          DX2_ReadLongData(dvid, id, 52, v32ccw, err);
        if result then
        begin
          cw := int32(v32cw);
          ccw := int32(v32ccw);
          if cw > ccw then
          begin
            ScrollBar1.Max := cw;
            ScrollBar1.Min := ccw;
          end else
          begin
            ScrollBar1.Max := ccw;
            ScrollBar1.Min := cw;
          end;
        end;
      end;
    else
    end;

  end;
end;

// トルクイネーブルと目標位置指令
function TForm1.WritePos (dvid:TDeviceID; id:uint8; pos:int32; var err:TErrorCode):boolean;
begin
  result := false;

  if AllActive then
  begin
    case ComboBox1.ItemIndex of
      0:  //XL
      begin
        result := DX2_WriteByteData(dvid, id, 24, 1, err)
          and DX2_WriteWordData(dvid, id, 30, ScrollBar1.Position, err);
      end;
      1:  //XM
      begin
        result := DX2_WriteByteData(dvid, id, 64, 1, err)
          and DX2_WriteLongData(dvid, id, 116, ScrollBar1.Position, err);
      end;
      2:  //PRO
      begin
        result := DX2_WriteByteData(dvid, id, 562, 1, err)
          and DX2_WriteLongData(dvid, id, 596, ScrollBar1.Position, err);
      end;
      3:  //PRO+
      begin
        result := DX2_WriteByteData(dvid, id, 512, 1, err)
          and DX2_WriteLongData(dvid, id, 564, ScrollBar1.Position, err);
      end;
    else
    end;

  end;
end;

// 現在位置取得
function TForm1.ReadPos (dvid:TDeviceID; id:uint8; var pos:int32; var err:TErrorCode):boolean;
var
  v16:uint16;
  v32:uint32;
begin
  result := false;
  if AllActive then
  begin
    case ComboBox1.ItemIndex of
      0:
      begin
        result := DX2_ReadWordData(dvid, id, 37, v16, err);
        pos := v16;
      end;
      1:
      begin
        result := DX2_ReadLongData(dvid, id, 116, v32, err);
        pos := v32;
      end;
      2:
      begin
        result := DX2_ReadLongData(dvid, id, 611, v32, err);
        pos := v32;
      end;
      3:
      begin
        result := DX2_ReadLongData(dvid, id, 580, v32, err);
        pos := v32;
      end;
    else
    end;
  end;
end;

// ポートオープン・クローズ
procedure TForm1.Button1Click(Sender: TObject);
var
  portname: ansistring;
begin
  // ポートオープン
  if (devid = 0) and DX2LIB_Active then
  begin
    portname := '\\.\'+ComboBox2.Text;
    devid := DX2_OpenPort(PAnsiChar(portname), StrToInt(ComboBox3.Text));
    if AllActive then
    begin
      Button1.Caption := 'close';
      Timer1.Enabled := True;
      ComboBox1.Enabled := false;
      ComboBox2.Enabled := false;
    end else
    begin
      beep;
      Timer1.Enabled := False;
    end;
  end else
  // ポートクローズ
  begin
    if AllActive then DX2_ClosePort(devid);
    Button1.Caption := 'open';
    devid := 0;
    Timer1.Enabled := False;
    ComboBox1.Enabled := true;
    ComboBox2.Enabled := true;
  end;
end;

// デバイスタイプの変更
procedure TForm1.ComboBox1Change(Sender: TObject);
begin
  Button1.Enabled := (ComboBox1.ItemIndex <> -1);
end;

// ボーレートの変更
procedure TForm1.ComboBox3Change(Sender: TObject);
begin
  if AllActive then
  begin
    DX2_SetBaudrate(devid, StrToInt(ComboBox3.Text));
  end;
end;

// スクロールバーのスライドで位置指令
procedure TForm1.ScrollBar1Change(Sender: TObject);
var
  err: TErrorCode;
begin
  WritePos (devid, StrToInt(ComboBox4.Text), ScrollBar1.Position, err);
  Label1.Caption := 'write errstat:$' + IntToHex(err, 4);
end;

// タイマで現在位置取得
procedure TForm1.Timer1Timer(Sender: TObject);
var
  err: TErrorCode;
  pos:int32;
begin
  if AllActive then
  begin
    ReadMaxMinPos (devid, StrToInt(ComboBox4.Text));
    if ReadPos(devid, StrToInt(ComboBox4.Text), pos, err) then
      Label3.Caption := 'present position:' + IntToStr(pos)
    else
      Label3.Caption := 'present position:?';
    Label2.Caption := 'read errstat:$' + IntToHex(err, 4);
  end else
  begin
    Timer1.Enabled := False;
    Label3.Caption := 'present position:-';
  end;
end;

end.
