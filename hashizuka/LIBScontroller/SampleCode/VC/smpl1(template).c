/*
  ���C�u�������g�p���邽�߂̍��q
*/

#include <stdio.h>    // ���������Ƃ��Ƃ肠�����C���N���[�h
#include "dx2lib.h"   // Dynamixel���C�u�����̃w�b�_

// �}�N����`
#define COMPORT   "\\\\.\\COM8" // �|�[�g�ԍ�
#define BAUDRATE  (1000000)     // �{�[���[�g[bps]

void main (void) {
  // �f�o�C�XID (COM�|�[�g���̃��j�[�N�l)
  TDeviceID  dev;

  // �w�肳�ꂽ�p�����[�^�Ń|�[�g���J��
  // ���������0�ȊO�̃��j�[�N�Ȓl(�f�o�C�XID)���Ԃ����
  // �Ȍケ�̃f�o�C�XID���g�p����
  dev = DX2_OpenPort (COMPORT, BAUDRATE);
  // �I�[�v������ɒʐM���J�n����Ƌ�����������I/F������̂�1�b���x�҂��Ƃ������߂���
  // ���Ȃ����ł���΍폜���č\��Ȃ�
  Sleep(1000);

  // dev��0�łȂ���΃|�[�g���J���̂ɐ���
  if (dev) {
    printf ("Open success\n");

    // ----�����ɂ�肽����������------------


    // --------------------------------------

    // �g���I������|�[�g�͕���
    DX2_ClosePort (dev);

  } else {
    printf ("Open error\n");
  }
  printf ("Fin\n");

  // �v���O�����������Ȃ�I�����Ȃ��l�A�L�[���͑҂���������
  getchar ();
}
