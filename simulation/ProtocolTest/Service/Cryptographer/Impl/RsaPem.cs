using System.Security.Cryptography;
using System.Text;

namespace ProtocolTest.Service.Cryptographer.Impl
{
    public static class RsaPem
    {
        public static (string publicPem, string privatePem) GenerateRsaKeyPair(int keySize = 2048)
        {
            using var rsa = RSA.Create(keySize);

            // Export to PKCS#8 (private) and SubjectPublicKeyInfo (public) as PEM
            var privatePkcs8 = rsa.ExportPkcs8PrivateKey();
            var publicSpki = rsa.ExportSubjectPublicKeyInfo();

            string privatePem = PemEncode("PRIVATE KEY", privatePkcs8);
            string publicPem = PemEncode("PUBLIC KEY", publicSpki);

            return (publicPem, privatePem);
        }

        public static RSA FromPublicPem(string publicPem)
        {
            var rsa = RSA.Create();
            rsa.ImportFromPem(publicPem);
            return rsa;
        }

        public static RSA FromPrivatePem(string privatePem)
        {
            var rsa = RSA.Create();
            rsa.ImportFromPem(privatePem);
            return rsa;
        }

        private static string PemEncode(string label, byte[] der)
        {
            var b64 = Convert.ToBase64String(der);
            var sb = new StringBuilder();
            sb.AppendLine($"-----BEGIN {label}-----");
            for (int i = 0; i < b64.Length; i += 64)
                sb.AppendLine(b64.Substring(i, Math.Min(64, b64.Length - i)));
            sb.AppendLine($"-----END {label}-----");
            return sb.ToString();
        }
    }
}