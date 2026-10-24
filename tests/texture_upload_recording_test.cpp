// Uses the maintained recording Vulkan boundary from the residency fixture.
int main(){
 using namespace pt;
 const auto cache=std::filesystem::temp_directory_path()/"pt-upload-batch-recording";
 std::filesystem::remove_all(cache);
 std::array<bool,16> native{};ConfigureBcConversion(native,cache,0);
 std::vector<uint8_t> red={0,248,0,248,0,0,0,0},white(4,255);
 TextureMip bc{4,4,red},builtin{1,1,white};
 {
  native_bc=true;TextureManager m;
  m.Create("white",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});
  auto index=m.Create("native",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1});
  assert(index==1&&m.converted_images_.empty()&&m.fallback_bytes_==0);
  auto&r=records.at(m.images_[index].image);assert(r.format==VK_FORMAT_BC1_RGBA_UNORM_BLOCK&&r.bytes[0]==red);
  auto before=format_queries;
  assert(m.Create("native",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1})==index&&format_queries==before);
 }
 {
  native_bc=false;TextureManager m;
  for(int i=0;i<4;++i)m.Create("builtin"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});
  std::array<TextureMip,18> cube;
  for(size_t f=0;f<6;++f)for(size_t l=0;l<3;++l)cube[f*3+l]={4u>>l,4u>>l,red};
  auto index=m.Create("cube",VK_FORMAT_BC1_RGBA_SRGB_BLOCK,cube,6,true);assert(index==4);
  auto&r=records.at(m.images_[index].image);assert(r.format==VK_FORMAT_R8G8B8A8_SRGB&&r.levels==3&&r.layers==6);
  for(size_t f=0;f<6;++f)for(size_t l=0;l<3;++l){auto&d=r.bytes[f*3+l];assert(d.size()==(4u>>l)*(4u>>l)*4);for(size_t i=0;i<d.size();i+=4)assert(d[i]==255&&d[i+1]==0&&d[i+2]==0&&d[i+3]==255);}
  auto bad=bc;bad.width=0;assert(m.Create("bad",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bad,1})==0);
  // Current residency policy preserves the live cube and gives over-budget
  // conversions stable deferred slots instead of the former permanent rejection.
  m.texture_use_={index};m.residency_limit_=m.fallback_bytes_;
  const auto retained=m.images_[index].image;const auto bytes=m.fallback_bytes_;
  auto deferred=m.Create("bounded",VK_FORMAT_BC1_RGBA_UNORM_BLOCK,{&bc,1});
  assert(deferred==5&&!m.images_[deferred].image&&m.images_[index].image==retained&&m.fallback_bytes_==bytes);
 }
 {
  native_bc=true;submissions=0;TextureManager m;m.BeginUploadBatch();
  std::vector<uint8_t>d(2048*1024*4,17);TextureMip large{2048,1024,d};
  for(int i=0;i<3;++i){d[0]=uint8_t(i);m.Create("batch"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&large,1});}
  assert(submissions==1&&m.upload_pending_.size()==1&&buffers.size()==1);
  m.EndUploadBatch();assert(submissions==2&&buffers.empty());
  for(int i=0;i<3;++i)assert(records.at(m.images_[i].image).bytes[0][0]==i);
 }
 {
  TextureManager m;submissions=0;m.BeginUploadBatch();m.BeginUploadBatch();
  for(int i=0;i<70;++i)m.Create("small"+std::to_string(i),VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});
  assert(submissions==1);m.EndUploadBatch();assert(submissions==1);
  m.EndUploadBatch();assert(submissions==2&&buffers.empty());
 }
 {
  TextureManager m;fail_buffer=true;
  assert(m.Create("allocation",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1})==0&&records.empty());
  fail_buffer=false;fail_submit=true;
  assert(m.Create("queue",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1})==0&&records.empty()&&buffers.empty());
  m.BeginUploadBatch();m.Create("queued",VK_FORMAT_R8G8B8A8_UNORM,{&builtin,1});
  bool threw=false;try{m.EndUploadBatch();}catch(const std::runtime_error&){threw=true;}assert(threw);
  fail_submit=false;m.FlushUploads();
 }
 assert(buffers.empty()&&records.empty());std::filesystem::remove_all(cache);
 puts("Production uploads passed: native compressed bytes, sRGB/mip/cube fallback bytes, deferred residency cap, bounded staging, nested batches and allocation/submission failures");
}
