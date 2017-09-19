.PHONY: all
all: bin/Release/netstandard2.0/Omegaup.Transact.dll

bin/Release/netstandard2.0/Omegaup.Transact.dll: Message.cs Interface.cs
	dotnet build

.PHONY: clean
clean:
	rm -rf bin/ obj/
