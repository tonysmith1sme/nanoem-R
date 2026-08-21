// 概要: シフトJISで書かれたエフェクト（日本語コメントの確認）
// 全角文字と\マークの混在テスト
float4 g_param <
  string UIName = "拡散強度";
  string UIHelp = "日本語ヘルプ";
> = float4(1, 1, 1, 1);
float4 vs_main(float4 position : POSITION) : POSITION { return position; }
float4 ps_main() : COLOR0 { return g_param; }
technique shift_jis {
  pass main {
    VertexShader = compile vs_3_0 vs_main();
    PixelShader = compile ps_3_0 ps_main();
  }
}
